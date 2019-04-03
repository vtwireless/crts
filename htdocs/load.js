// This file must be served from the web server document root.
//


// javaScript load function
//
// Usage:
//
//    <script src="/load.js?/module1.js+/foo/module2.js+/module3.js"></script>
//
//    Will be kind-of (functionally) like:
//
//   <script src="/load.js"></script>
//   <script src="/module1.js"></script>
//   <script src="/foo/module3.js"></script>
//   <script src="/module3.js"></script>
//
//
// Except we load the javaScript in this javaScript.
//
// This leaves us the ability to, in the future, optimise by having the
// web server send all the javaScript files (example:
// /module1.js,/foo/module2.js,/module3.js) in one compressed file.
// For HTTP2 this would be not so different than just compressing
// each file separately since HTTP2 can group multiple GET requires
// together, and so there would be just a little more HTTP header data
// sent.
//
// Any javaScript along the way may call require() and load more files.
//
// Why are we not using react compile the javaScript?  Give me a break,
// this less than 200 lines of javaScript.  We don't need to start over
// adding lot of sugar coated javaScript.  We are trying to keep things
// straight forward.
//


// Developer friendly utility functions fail() and assert() are not so
// user friendly.
//
function fail() {

    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    line = '\n--------------------------------------------------------';
    text += line + '\nCALL STACK' + line + '\n' +
        new Error().stack + line;
    console.log(text);
    alert(text);
    stop();
    // It makes no sense that we can throw after calling stop(),
    // but it works.
    throw text;
}


function assert(val, msg=null) {

    if(!val) {
        if(msg)
            fail(msg);
        else
            fail("JavaScript failed");
    }
}


// Just a wrapper of document.getElementById(), for in case you need to
// stop running if it fails.
//
function getElementById(id) {

    var element = document.getElementById(id);
    if(!element) fail("document.getElementById(" + id + ") failed");
    return element;
}


function require(url, callback = function() {}) {

    // So long as there are pending urls to load (onload), no callbacks will be
    // called.  The urls are loaded one at a time in order that this is
    // called.  If any script url fails to load in a timeout than this
    // will stop the program.
    //
    // If this is called at a later time from within a javaScript function
    // you get what you paid for.
    //
    // TODO: make this efficient, and this compile the javaScript into
    // one compressed file, to server the next time.

    require.pendingCallbacks.push(callback);

    function callCallbacks() {
        console.log('CALLING require() CALLBACKS');
        while(require.pendingCallbacks.length > 0)
            require.pendingCallbacks.shift()();
    }

    function getFileType(src) {

        let file = src.replace(/\?.*$/, ''); // strip off the query
        if(file.substr(file.length-3) === '.js') return 'js';
        if(file.substr(file.length-4) === '.css') return 'css';
        fail('Unknown file type for src=' + src);
    }



    function load() {

        if(require.pendingSrcs.length === 0) {
            // We could still be waiting on the
            // last one to load.
            if(!require.waiting)
                callCallbacks();
            return;
        }

        if(require.waiting) 
            // We wait and call load() after the script it loaded.
            return;

        require.waiting = true;

        // TODO: Should we change this timeout value???
        const timeoutSecs = 10;


        var timeout = setTimeout(function() {
            require.waiting = false;
            fail("failed to GET script " + src + " in " +
                timeoutSecs + " seconds");
        }, timeoutSecs*1000/* milliseconds  1/1000*/);

        let src = require.pendingSrcs.shift();
        console.log("START loading: " + src);

        function onLoad() {

            require.waiting = false;
            console.log('FINISHED loading: ' + src);
            clearTimeout(timeout);
            // recurse
            load();
        }

        let content;

        switch(getFileType(src)) {
            case 'js':
                content = document.createElement('script');
                content.src = src;
                content.onload = onLoad;
                break;
            case 'css':
                content = document.createElement('link');
                content.setAttribute("rel", "stylesheet")
                content.setAttribute("type", "text/css")
                content.setAttribute("href", src)
                content.onload = onLoad;
                break;
            default:
                fail('require(url="' + url + '")');
        }
        document.head.append(content);
    }


    // We must canonicalize the path part of the URL so that it is
    // unique to a url path on the web server.  We'll assume that all
    // files come from the same server.  There is no way short of
    // adding code to the javaScript files to get a unique path of
    // running javaScript.  The Error().stack gives a URL for the
    // javaScript that calls this function, but that is not
    // necessarily unique for a given javaScript file on the server.
    // The client browser only knows that the javaScript came from a
    // URL and that URL is not unique, unless we impose rules on what
    // the form of the argument url in require(url).  So we
    // require that the path part of the url be a "full server root"
    // path.
    //
    // Example: url =
    //
    //    '/paTh/to/file.js?query+bla+blla'
    //
    //    p = /paTh/to/file.js
    //
    // strip chars after the path
    //
    // add a file path prefix if this started this page by loading using
    // local file system urls like: file:///path/to/file.js
    // otherwise require.rootDir is an empty string.
    //
    url = require.rootDir + url;

    var p = url.replace(/\?.*$/, '');

    //console.log('p=' + p);

    // It can not have /../ in it and it must be a full path.
    if(p.match(/\/\.\.\//) !== null || p.substr(0,1) !== '/')
        fail('Bad path in URL argument to require(url="' + url + '")');

    if(require.paths.findIndex(function(path) {
        return p === path;
        }) >= 0) {
        console.log('found javaScript[' + url +
            '] path "' + p + '" is ALREADY loaded');
    } else {
        console.log('adding: ' +  p);
        require.paths.push(p);
        require.pendingSrcs.push(url);
    }

    load();
}


assert(require.paths === undefined, "this this was loaded twice");

// Initialize some static variables for the function require().  It'd be
// nice if they where private too.
require.paths = []; // array of unique paths associated with src (url)
require.pendingCallbacks = [];
require.pendingSrcs = [];
require.waiting = false;


(function() {


    let count = 0;
    let scripts = document.getElementsByTagName('script');
    var src = scripts[scripts.length-1].src;

    // We assume that the root directory is where this file is.
    require.rootDir = src.
        replace(/^http[s]*\:\/\/[^\/]*/, '').
        replace(/^file:\/\/\//, '/').
        replace(/\/load\.js.*$/,'');

    // examples:
    //
    //     src = file:///foo/bar/load.js?asdfasdf  -> require.rootDir = '/foo/bar'
    //
    //     src = https://foo.com:8080/load.js?asdf -> require.rootDir = ''
    //
    //
    // We use require.rootDir so we can load javaScript without a server.
    //
    if(require.rootDir === '/') require.rootDir = '';

    console.log('require.rootDir=' + require.rootDir);

    let urls = src.replace(/^.*\?/,'').split('+');

    urls.forEach(function(url) {
        require(url);
    });
})();
