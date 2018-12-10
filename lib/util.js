// We define some global utility so that that can be used by all modules
// in the program.  We don't even need to export these functions, because
// they are define as globals like so:
//


fail = function() {

    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    const line = '\n--------------------------------------------------------';
    text += line + '\nCALL STACK' + line + '\n' +
        new Error().stack + line;

    console.log(text);
    var process = require('process');
    process.exit(1);
};


assert = function(val, msg=null) {

    if(!val) {
        if(msg)
            fail(msg);
        else
            fail("JavaScript failed");
    }
};


var path = require('path'),
    fs = require('fs');

rmDirRecursive = function(fpath) {
    if(fs.existsSync(fpath)) {
        fs.readdirSync(fpath).forEach(function(file, index){
        var curPath = path.join(fpath, file);
        if(fs.lstatSync(curPath).isDirectory()) // recurse
            rmDirRecursive(curPath);
        else // delete file
            fs.unlinkSync(curPath);
        });
        fs.rmdirSync(fpath);
    }
    assert(fs.existsSync(fpath) === false);
};
