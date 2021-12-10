require('/controllers.js');



// Display a parameter as a number.
//
//
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
function Label(controllers, program, filter, parameter, output=null,
        opts = {}) {

    // Defaults:
    var digits = 3;
    var prefix = null;
    var suffix = null;
    var scale = 1.0;

    if(opts.digits !== undefined)
        digits = opts.digits;

    if(typeof(output) === "string")
        // We'll assume that this is a CSS Selector string
        // for a page with HTML already built.
        output = document.querySelector(output);

    if(!output) {
        output = document.createElement("output");
        let div = document.createElement("div");
        div.appendChild(output);
        document.body.appendChild(div);
    }



    function initParameter(par) {

        input.min = par.min;
        input.max = par.max;
        input.step = (input.max - input.min)/1000;
        input.onchange = function() {
            // This will send a request through the web socket to change
            // the value via the parameter setter (=).
            par.set(parseFloat(input.value).toPrecision(digits));
        };

        console.log("par=" + JSON.stringify(par));

        par.addOnChange(function(value) {
            input.value = value;
            //console.log("setting " + parameter + "=" + value);
        });
    }

    // This will finish setting up this slider to act with the web socket
    // data as it changes a parameter.  If the parameter is never defined
    // on the server, it is never set up.
    //
    // initParameter() is an event handler that is called if the parameter
    // with name "program" "filter" "parameter" comes into existence.
    //
    //controllers.parameter(initParameter, program, filter, parameter);
}

