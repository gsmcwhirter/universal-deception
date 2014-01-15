var args = require("optimist")
      .usage("$0 --stats <statsfile> --out <outfile> --payoffs <payoffs>")
      .demand(["out", "stats", "payoffs"])
      .alias({"out": "o", "stats": "s", "payoffs": "p"})
      .describe({ "out": "The file to output with data."
                , "stats": "The stats file to parse."
                , "payoffs": "The payoff scheme used for the simulations (1--3)."})
      .argv
  , fs = require("fs")
  , path = require("path")
  , _ = require("underscore")
  , lib = require("./stats_lib")
  ;

var data = require(path.resolve(args.stats));
var wstream = fs.createWriteStream(args.out);
wstream.on("error", function (err){
  console.log("Write stream error:");
  console.log(err);
});

wstream.on("open", function (){
  var stats = parseStats(data);
  wstream.end(JSON.stringify(stats, null, 2), "utf8", function (){
    console.log("Stats written successfully.");
  });
});

function parseStats(data){
  var ret = {
      pure5or14: {}
    , pureOther: {}
    , mixed: {}
    }
  ;

  var pures = {};
  var mixed_combos = {};

  var mixed_key;

  data.forEach(function (dup){
    if (!dup) return;

    var key;

    if (dup.data.length === 1){
      //pure equilibrium

      if (dup.data[0].strategy === 5 || dup.data[0].strategy === 14){
        ret.pure5or14[dup.file] = 'pure5or14';
        if (!pures[dup.data[0].strategy]){
          pures[dup.data[0].strategy]  = 0;
        }

        pures[dup.data[0].strategy] += 1;
      }
      else {
        ret.pureOther[dup.file] = 'pureOther';
        if (!pures[dup.data[0].strategy]){
          pures[dup.data[0].strategy] = 0
        }

        pures[dup.data[0].strategy] += 1;
      }
    }
    else {
      mixed_key = dup.data.map(function (datum){
        return datum.strategy;
      }).sort().join(",");

      ret.mixed[dup.file] = mixed_key;
      if (!mixed_combos[mixed_key]){
        mixed_combos[mixed_key] = {count: 0, pcts_deception: []};
      }

      mixed_combos[mixed_key].count += 1;
      mixed_combos[mixed_key].pcts_deception.push(lib.percentDeception(dup.data, args.payoffs));
    }
  });

  var counts = {};
  for (var stat in ret){
    counts[stat] = Object.keys(ret[stat]).length;
  }

  return [ret, counts, pures, mixed_combos];
}