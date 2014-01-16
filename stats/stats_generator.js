var args = require("optimist")
      .usage("$0 --out <outfile> --dir <results_dir> [--tol <tolerance>]")
      .demand(["out", "dir"])
      .alias({"out": "o", "dir": "d", "tol": "t"})
      .default("tol", 0.0005)
      .describe({ "out": "The file to output with statistics."
                , "dir": "The directory, tar, or tar.gz containing the .out files from a simulation run."
                , "tol": "Tolerance for counting as non-zero."})
      .argv
  , fs = require("fs")
  , path = require("path")
  , events = require("events")
  , util = require("util")
  , tar = require("tar")
  , gzbz = require("gzbz/streaming")
  ;      

require("buffertools"); //monkey-patch

function FilePond(files, action, maxConcurrent){
  this.queue_pointer = 0;
  this.file_queue = files;
  this.action = action;
  this.maxConcurrent = maxConcurrent || 10;
  this._doneCount = 0;
  
  var self = this;
  
  function _whenDone(err){
    self._doneCount += 1;
    //console.log("Done %s of %s.", self._doneCount, self.file_queue.length);
    
    if (err){
      self.emit("error", err);
    }
    
    if (self._doneCount >= self.file_queue.length){
      self.emit("end");
    }
    else if (self.queue_pointer < self.file_queue.length){
      var nextFile = self.unshiftFile();
      
      if (nextFile){
        process.nextTick(function (){
          self._startAction(nextFile);
        });  
      }
      else {
        self.emit("error", {file: nextFile, error: "No next file. Returned " + nextFile + "."});
      }
    }
  }
  
  action.on("data", function (data){ 
    self.emit("data", data);
  });
  
  action.on("end", _whenDone);
  action.on("error", _whenDone);
}

util.inherits(FilePond, events.EventEmitter);

FilePond.prototype._startAction = function (file){
  if (file){
    this.emit("file", file);
    this.action.act(fs.createReadStream(file), file);
  }
  else {
    this.emit("error", {file: "N/A", error: "Tried to dispatch file: " + file});
  }
};

FilePond.prototype.unshiftFile = function (){
  var ret = this.file_queue[this.queue_pointer];
  this.queue_pointer += 1; 
  return ret;
};

FilePond.prototype.run = function (){
  var self = this;
  for (var i = 0; i < this.maxConcurrent; i++){
    if (this.file_queue.length && this.queue_pointer < this.file_queue.length){
      process.nextTick(function (){
        self._startAction(self.unshiftFile());
      });
      //console.log("dispatched %s.", i);
    }
  }
};

function Action(){}
util.inherits(Action, events.EventEmitter);
Action.prototype.act = function (rstream){
  this.emit("end");
};

function StatsAction(){}
util.inherits(StatsAction, Action);
StatsAction.prototype.act = function (rstream, filename){
  var self = this;
  
  var end_state_started = false
    , start_byte = 0
    , incomplete_line = ""
    , proportions = []
    , data_events = false
    ;
  
  rstream.on("readable", function (){
    var buffer = rstream.read();    
    parseBuffer(buffer);
  });

  rstream.on("data", function (buffer){
    parseBuffer(buffer);
  });

  function parseBuffer(buffer, force_incline){
    data_events = true;

    if (!buffer && !force_incline) return;

    var match = buffer.toString().match(/Done simulation\.\n/g);

    if (end_state_started){
      start_byte = 0;
    }
    else if (match && match.length > 0) {
      end_state_started = true;
      start_byte = buffer.indexOf("Done simulation.\n");
    }
    
    if (end_state_started){
      var lines = buffer.toString('utf8', start_byte).split("\n");
      lines[0] = incomplete_line + lines[0];
      incomplete_line = lines.pop();
      
      lines.forEach(function (line){
        var matches;
        matches = line.match(/^\s*Strategy (\d+) \((.+)\):\s*$/);
        if (matches && matches.length){
          var strategy = parseInt(matches[1].trim(), 10);
          proportions.push({strategy: strategy, proportion: parseFloat(matches[2].trim())});
        }
      });
    }
  }

  function parseProportions(proportions){
    var tolerance = args.tol || 0.0005;

    var retData = [];

    if (!proportions || proportions.length === 0){
      return retData;
    }
      
    var states = 2
      , messages = 2
      , actions = 3
      ;
    
    var i, j;
        
    proportions.forEach(function (prop, q){
      if (prop.proportion > tolerance){
        retData.push(prop);
      }
    });

    return retData;
  } 
  
  rstream.on("error", function (err){
    self.emit("error", {error: err, file: filename});
  });
  
  rstream.on("end", function (){
    if (data_events){
      parseBuffer(new Buffer(""), true);
      self.emit("data", {data: parseProportions(proportions), file: filename});
    }
    self.emit("end");
  });
};

var wstream
  , go = function (){
      console.log("Not overwritten.");
      wstream.end();
    }
  ;

if (fs.statSync(args.dir).isDirectory()){
  fs.readdir(args.dir, function (err, files){
    if (err){
      console.log("Error reading the output directory.");
      return;
    }
    
    console.log("Opened the output directory.");

    var pond = new FilePond(files.map(function (file){ return path.join(args.dir, file); }), new StatsAction());

    console.log(pond.file_queue.length);
    
    pond.on("file", function (file){
      console.log("Parsing %s...", file);
    });
    
    pond.on("error", function (err){
      console.log("Error on %s:", err.file);
      console.log(err.error);
    });
    
    pond.on("data", dataWriter);
    
    pond.on("end", function (){
      endWStream();
      console.log("done.");
    });
    
    go = function (){
      pond.run();
    };  

    actuallyStart();
  });
}
else {
  //assume it is a tar file
  var action2 = new StatsAction();
  action2.on("data", dataWriter);

  go = readTar;

  function readTar(withGz){
    var rstream = fs.createReadStream(args.dir);
    if (withGz){
      rstream = rstream.pipe(new gzbz.GzipInflater());
    }

    rstream.pipe(tar.Parse())
      .on("entry", function (entry){
        action2.act(entry, entry.props.path);
      })
      .on("error", function (err){
        if (!withGz){
          readTar(true);
        }
        else {
          throw err;
        }
      })
      .on("end", function (){
        endWStream();
        console.log("done.");
      })
      ;
  }

  actuallyStart();
}

function actuallyStart(){
  wstream = fs.createWriteStream(args.out);
  wstream.on("error", function (err){
    console.log("Write stream error:");
    console.log(err);
  });

  wstream.on("open", function (){
    wstream.write("[\n");
    go();
  });
}

function dataWriter(data){
  console.log("Writing data for %s", data.file);
  
  //wstream.write(inspect(data, {depth: 4, colors: false}));
  wstream.write(JSON.stringify(data, null, 2));
  wstream.write(",\n\n");
}

function endWStream(){
  wstream.end("null]\n", "utf8", function (){
    var jsdata = require(path.resolve(args.out));
    console.log(jsdata.length);
  });
}