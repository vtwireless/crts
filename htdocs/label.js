require('/controllers.js');
require('/d3.v5.min.js'); // for d3.format(".2f") ... etc


// Display a parameter as a number.
//
//
// Looks like so:
//
//    prefix number*scale suffix
//
// For example:
//
//    freq 205 MHz
//
//   output:  HTML output element, or querySelector string to an
//            HTML output element, or null to append a newly created div
//            to the bottom of page body.
//
//
// options arg:
//
//    opts:
//
//         digits: integer number of digits to display for number
//         prefix: string or not set
//         suffix: string or not set
//         scale:  float to multiple the parameter value by
//                 to get a number to display
//
//
//
function Label(parameter, opts = {}) {


    function getOpt(x, defaultVal) {
        if(opts[x] !== undefined)
            return opts[x];
        return defaultVal;
    };


    var digits     = getOpt('digits',     4     );
    var prefix     = getOpt('prefix',     ''    );
    if(!prefix)
        prefix     = getOpt('label',      ''    );
    var step       = getOpt('step',       false );
    var scale      = getOpt('scale',      1.0   );
    var suffix     = getOpt('suffix',     ''    );
    var parentNode = getOpt('parentNode', null  );


    if(typeof(parentNode) === "string") {
        let str = parentNode;
        // We'll assume that this is a CSS Selector string
        // for a page with HTML already built.
        parentNode = document.querySelector(str);
        if(!parentNode)
            throw("Slider element parent \"" + str + '" was not found');
        if(parentNode.className === undefined)
            parentNode.className = 'slider';
    } else if(parentNode === null) {
        parentNode = document.createElement("div");
        parentNode.className = 'slider';
        document.body.appendChild(parentNode);
    } // Else
      // the user passed in a parentNode that we will try to use.


    var output = document.createElement("output");
    output.className = 'label';
    parentNode.appendChild(output);


    function Value(value) {
        return d3.format('.' + digits + 'f')(value * scale);
    }


    function initParameter(par) {

        par.addOnChange(function(value) {
            output.value = prefix + Value(value) + suffix;
        });
    }

    parameter.onSetup(initParameter);
}

