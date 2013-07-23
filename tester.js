var fs = require('fs');
var assert = require('assert');

// usage: node tester.js /tmp/target /tmp/base
// reads random place from a file in target and checks it against base

console.log(process.argv);
assert(process.argv.length == 4)

var target = process.argv[2];
var check = process.argv[3];


function test_file(name) {
    var buf = new Buffer(256);
    var buf_check = new Buffer(256);
    fs.stat(check + name, function(err, stats) {
	if(err) throw err;
	fs.open(target + name, 'r', function (err, target_file) {
	    if(err) throw err;
	    fs.open(check + name, 'r', function(err, check_file) {
		if(err) throw err;
		function run_check (place, length) {
		    fs.read(target_file, buf, 0, length, place, function (err, read_bytes, buf_back) {
			if(err) throw err;
			fs.read(check_file, buf_check, 0, length, place, function(err, read_bytes2, buf_back2) {
			    if(err) throw err;

			    if(buf.toString('utf-8', 0, length) == buf_check.toString('utf-8', 0, length)) {
				console.log('passed check:',name,place);
			    }else{
				console.error('>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> FAILED CHECK:', name, place);
			    }
			    run_check(Math.floor((stats.size - 500) * Math.random()), Math.floor(256 * Math.random()));
			});
		    });
		}
		run_check(0, 100);
	    });
	});
    });
}


function start_tests(name) {


}



start_tests('/');

test_file('/96f783f2bba6be1d5a6c360b5ee1dce4bb2be5ea/[HorribleSubs] Naruto Shippuuden - 320 [720p].mkv');
