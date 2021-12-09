require('/controllers.js');





function Slider(controllers, program, filter, parameter, input=null,
        opts = {}) {


    var digits = 3;

    if(opts.digits !== undefined)
        digits = opts.digits;

    if(typeof(input) === "string")
        // We'll assume that this is a CSS Selector string
        // for a page with HTML already built.
        input = document.querySelector(input);

    if(!input) {
        input = document.createElement("input");
        input.type = "range";
        let div = document.createElement("div");
        div.appendChild(input);
        document.body.appendChild(div);
    }

    if(input.type !== "range")
        throw("Cannot setup slider from input of type: " + input.type);


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
    controllers.parameter(initParameter, program, filter, parameter);
}

