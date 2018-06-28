//
// p - object with list of parameters that gets added to.
//
// requires that <table id=inputTable></table> exists.
//
function FunctionTest_init(p, inputTable) {

    var obj = {};

    function FindChildren(node, func) {
        for(let child=node.firstChild; child; child=child.nextSibling) {
            func(child);
            FindChildren(child, func);
        }
    }

    function AppendParameterText(text, parameter) {
        var val = parameter.value;
        text.data += parameter.Name + '=';
        spew(' ----------------------------- ' + (typeof val));

        if(Array.isArray(val))
            text.data += '[';
        else if(typeof val === 'string')
            text.data += '"';

        text.data += val;

        if(typeof val === 'string')
            text.data += '"';
    }

    function UpdateFunctionText(node) {

        let text = node.firstChild;

        spew(' --------------- changing :' + text.data + '   Args=' + node.Args);

        text.data = node.FunctionName + '(';
        let gotData = false;
        node.Args.forEach(function(parameter) {
            if(gotData)
                text.data += ',';
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
    obj.CreateInput = function(id, par) {

        // Array of functions that use this input parameter
        par.Functions = [];
        // This object needs it's key that it is named as.
        par.Name = id;

        let tr = document.createElement('TR');
        inputTable.appendChild(tr);

        let td = document.createElement('TD');
        tr.appendChild(td);
        td.appendChild(document.createTextNode(par.description));

        td = document.createElement('TD');
        tr.appendChild(td);

        let unitTd = document.createElement('TD');
        tr.appendChild(unitTd);
        unitTd.appendChild(document.createTextNode(par.units));

        if(par.container !== undefined) {
            par.container = td;
            // We have no input.  We let the user put stuff
            // in this <td> container.
            return;
        }

        let input = par.input = document.createElement('INPUT');

        if(par.readonly !== undefined && par.readonly) {
            input.readOnly = true;
        }

        td.appendChild(input);

        if(Array.isArray(par.value))
            input.value = JSON.stringify(par.value);
        else
            input.value = par.value.toString();


        function isInt(n){
            return Number(n) === n && n % 1 === 0;
        }

        function isFloat(n){
            return Number(n) === n && n % 1 !== 0;
        }

        // Setup all the text nodes to get text from this input.
        input.onkeyup = function(e) {
            if(Array.isArray(par.value)) {
                par.value = JSON.parse(input.value);
            } else if(isFloat(par.value)) {
                par.value = parseFloat(input.value);
            } else if(isInt(par.value)) {
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
    }

    // Initialize the inputs values
    Object.keys(p).forEach(function(key) {
        obj.CreateInput(key, p[key]);
    });


    obj.UpdateFunctions = function() {

        FindChildren(document.getElementsByTagName("BODY")[0], function(node) {

            if(node.className === 'functionButton' && node.firstChild &&
                node.firstChild.nodeType === Node.TEXT_NODE) {

                let tNode =  node.firstChild;
                let text = tNode.data;
                let functionName = text.replace(/\(.*$/, '');
                let args = text.replace(functionName + '(', '');
                args = args.replace(/\).*$/, '');
                spew('func() ==' + functionName + '(' + args + ')');
                node.FunctionName = functionName;
                node.Args = [];

                args.replace(/\s*/, '').split(',').forEach(function(arg) {

                    assert(p[arg] !== undefined && p[arg].value !== undefined,
                        'Bad arg: ' + arg + ' in function ' + functionName + '()');

                    // Add to the functions list of arguments.
                    node.Args.push(p[arg]);

                    // Add this function to the array of functions that use
                    // the parameter p[arg].
                    //
                    p[arg].Functions.push(node);
                });

                node.onclick = function() {
                    spew("TODO: need to call: " + functionName + '(argsArray=' + JSON.stringify(node.Args) + ')');
                };

                spew(' ---------------------- ' + functionName + '()');
                spew(' ---------------------- ' + functionName + '(argsArray=' + args + ')');
             }
        });
    }

    obj.UpdateFunctions();
}

