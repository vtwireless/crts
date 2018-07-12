//
// p - object with list of parameters that gets added to.
//
// inputTable - <table> element to append to.
//
function FunctionTest_init(p, inputTable) {

    function FindChildren(node, func) {
        for(let child=node.firstChild; child; child=child.nextSibling) {
            func(child);
            FindChildren(child, func);
        }
    }

    function AppendParameterText(text, parameter) {

        if(parameter.value === undefined || parameter.Name === undefined) {
            // This is not in our parameter list so we'll assume that
            // it is a window global variable with name/key parameter.
            //
            text.data += parameter;
            let val = window[parameter];
            assert(val !== undefined, parameter +
                    ' is not a window global varable' +
                    ' or in the parameter list');

            if(val.toString !== undefined) {
                text.data += '=';
                if(typeof val === 'string')
                    text.data += '"';
                text.data += val.toString();
                if(typeof val === 'string')
                    text.data += '"';
            }

            return;
        }

        var val = parameter.value;
        text.data += parameter.Name + '=';

        if(Array.isArray(val)) {
            text.data += JSON.stringify(val);
            return;
        }

        if(typeof val === 'string')
            text.data += '"';

        text.data += val;

        if(typeof val === 'string')
            text.data += '"';
    }

    function UpdateFunctionText(node) {

        let text = node.firstChild;

        text.data = node.FunctionName + '(';
        let gotData = false;
        node.Args.forEach(function(parameter) {
            if(gotData)
                text.data += ', ';
            else
                gotData = true;
            AppendParameterText(text, parameter);
        });
        text.data += ')';
    }


    // Create an input that sets parameters in all HTML nodes
    // keep a Args array of arguments.

    // Create an input and add it to the inputTable and make
    // obj have more object data in it.
    //
    function CreateInput(id, par) {

        // Array of functions that use this input parameter
        par.Functions = [];
        // This object needs it's key that it is named as.
        par.Name = id;


        let tr = document.createElement('TR');
        inputTable.appendChild(tr);

        let td = document.createElement('TD');
        tr.appendChild(td);
        td.className = 'parameterName';
        td.appendChild(document.createTextNode(par.Name))


        td = document.createElement('TD');
        tr.appendChild(td);
        td.appendChild(document.createTextNode(par.description));

        td = document.createElement('TD');
        tr.appendChild(td);

        if(par.container !== undefined) {
            par.container = td;
            // We have no input.  We let the user put stuff
            // in this <td> container.
            return;
        }

        let input = par.input = document.createElement('INPUT');

        td.appendChild(input);


        if(par.units !== undefined && par.units.length)
            td.appendChild(document.createTextNode(' ' + par.units));

        if(par.readonly !== undefined && par.readonly) {
            input.readOnly = true;
            let span = document.createElement('span');
            span.className = 'readonly';
            span.appendChild(document.createTextNode("readonly"));
            td.appendChild(span);
        }


        if(Array.isArray(par.value))
            input.value = JSON.stringify(par.value);
        else
            input.value = par.value.toString();


        // By keeping the initial value we can remember what type the
        // value is, so that we know how to get a value in the future.
        var initialValue = par.value;

        // Setup all the text nodes to get text from this input.
        input.onkeyup = function() {

            function isInt(n){
                return Number(n) === n && n % 1 === 0;
            }

            function isFloat(n){
                return Number(n) === n && n % 1 !== 0;
            }

            if(Array.isArray(initialValue)) {
                try {
                    par.value = JSON.parse(input.value);
                } catch(e) {
                    spew("bad functionTest <input> value: JSON.parse(" +
                        input.value + ') failed');
                };
            } else if(isFloat(initialValue)) {
                par.value = parseFloat(input.value);
            } else if(isInt(initialValue)) {
                par.value = parseInt(input.value);
            } else // string
                par.value = input.value;

            // update the list of functions and the arguments that are
            // displayed.
            par.Functions.forEach(function(node) {
                UpdateFunctionText(node);
            });
        };

        input.onchange = input.onkeyup;

        par.setValue = function(val) {

            // val needs to be a string.
            assert(typeof val === 'string',
                "par.setValue(val) val is not a string");
            par.input.value = val;
            // Simulate an onkeyup event for this <input>
            par.input.onkeyup();
        };

    }

    // Initialize the inputs values
    Object.keys(p).forEach(function(key) {
        CreateInput(key, p[key]);
    });


    function UpdateFunctions() {

        FindChildren(document.getElementsByTagName("BODY")[0], function(node) {

            if(node.className === 'functionButton' && node.firstChild &&
                node.firstChild.nodeType === Node.TEXT_NODE) {

                
                let tNode =  node.firstChild;
                let text = tNode.data;
                let functionName = text.replace(/\(.*$/, '').replace(/\s*/, '');
                let args = text.replace(functionName + '(', '').
                    replace(/\).*$/, '').replace(/\s*/g, '');

                node.FunctionName = functionName;
                node.Args = [];

                // Make an array of args:
                args.split(',').forEach(function(arg) {

                    if(p[arg] !== undefined && p[arg].value !== undefined) {
                        node.Args.push(p[arg]); // This p[arg] is in the parameter list
                        // Add this function to the array of functions that use
                        // the parameter p[arg].
                        //
                        p[arg].Functions.push(node);
                    } else
                        // This arg is not in our parameter list so we
                        // will use arg differently.
                        node.Args.push(arg);
                });

                node.onclick = function() {
                    // Construct the argument array now, on the fly, so
                    // that we get the latest value for the parameters.
                    var args = [];

                    node.Args.forEach(function(parameter) {
                        if(parameter.value !== undefined)
                            args.push(parameter.value);
                        else {
                            assert(window[parameter] !== undefined, parameter +
                                ' is not a window global varable' +
                                ' or in the parameter list');
                            args.push(window[parameter]);
                        }
                    });

                    spew("Running: " + functionName + tNode.data);

                    window[functionName](...args);
                };

                spew('making function caller for: ' + functionName + '(argsArray=[' + args + '])');
             }
        });
    }

    UpdateFunctions();

    // Simulate onkeyup events for all <input>s.
    Object.keys(p).forEach(function(key) {
        if(p[key].input !== undefined)
            p[key].input.onkeyup();
    });
}
