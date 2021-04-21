const min_freq = 918e6; // Hz
const max_freq = 921e6; // Hz

const min_freq1 = min_freq; // Hz
const max_freq1 = max_freq; // Hz
const min_freq2 = min_freq; // Hz
const max_freq2 = max_freq; // Hz

const min_bw = 0.4e6;   // Hz
const max_bw = 3.57e6;  // Hz

const min_bw1 = min_bw;  // Hz
const max_bw1 = max_bw; // Hz
const min_bw2 = min_bw;  // Hz
const max_bw2 = max_bw; // Hz

const min_gn1 = 0.0;  // dB
const max_gn1 = 31.5; // dB
const min_gn2 = 0.0;  // dB
const max_gn2 = 31.5; // dB

const min_mcs1 = 0.0;  // index
const max_mcs1 = 11.0; // index
const min_mcs2 = 0.0;  // index
const max_mcs2 = 11.0; // index



// The caller passes in the io object.
//
function SetupRunUserJavaScript(io) {


    var textArea = document.querySelector('#userfunction');

    var editor = CodeMirror.fromTextArea(
        textArea, {
            lineNumbers: true,
            theme: "blackboard",
            mode: {name: "javascript", globalVars: true}
        }
    );


    var checkbox = document.querySelector('#run');
    checkbox.checked = false;
    var running = false;

    var select = document.querySelector('#strategy');

    var intervalId;


    Object.keys(selectFunctions).forEach(function(funcName) {

        // Create the select options from the list of user functions.
        var opt = document.createElement('option');
        opt.value = funcName;
        opt.innerHTML = funcName;
        select.appendChild(opt);
    });


    var userFunction;

    var userData = {};

    select.onchange = function() {

        if(select.className !== 'notedited')
            select.className = 'notedited';

        var func = selectFunctions[select.value].toString();
        func = func.substring(func.indexOf("\n")+1);
        editor.setValue(func.substring(0, func.lastIndexOf("\n")));

        if(running)
            Stop();
    };

    // Initialize the userFunction editor.
    select.onchange();

    var parameters = {
        // These value should change and get re-initialized in
        // io.On('getParameter',,)
        freq1: 918e6,
        bw1: 0.5e6,
        gn1: 1.0,
        mcs1: 1.0,
        bytes1: 0.0,
        freq2: 918e6,
        bw2: 0.5e6,
        gn2: 1.0,
        mcs2: 1.0,
        bytes2: 0.0
    };


    const dtStr = '0.5';

    const dt = parseFloat(dtStr); // delta time in seconds.

    document.querySelector('#dt').appendChild(document.createTextNode(dtStr));


    var tx1_programName = null;
    var rx1_programName = null;
    var tx2_programName = null;

    var init = true;


    function callUserFunction() {

        if(!running)
            // The timer popped after we set the flag.
            return;

        if(!tx1_programName || !rx1_programName || !tx2_programName) 
            // TODO: we need a timeout here that waits for the
            // getParameter events to get the program names.
            return;

        var p = parameters;

        try {

            var ret = userFunction(p.freq1, p.bw1, p.gn1, p.mcs1, p.bytes1,
                    p.freq2, p.bw2, p.gn2, p.mcs2, p.bytes2, dt, userData, init);

        } catch(e) {

            if(running)
                Stop();
            alert("Can't run. Bad code\n\n" + e);
            return;
        }

        init = false;

        if(typeof ret !== 'object')
            return;

        function checkBounds(key, min, max) {
            if(ret[key] < min)
                ret[key] = min;
            else if(ret[key] > max)
                ret[key] = max;
        }


        Object.keys(ret).forEach(function(key) {
            // Write the returned values to the web server.
            switch(key) {
                case 'freq1':
                    checkBounds('freq1', min_freq1, max_freq1);
                    io.Emit('setParameter', tx1_programName, 'tx2', 'freq', ret.freq1);
                    io.Emit('setParameter', rx1_programName, 'rx2', 'freq', ret.freq1);
                    break;
                case 'freq2':
                    checkBounds('freq2', min_freq2, max_freq2);
                    // there is no receiver, rx, program for freq2
                    io.Emit('setParameter', tx2_programName, 'tx2_interferer', 'freq', ret.freq2);
                    break;
                case 'gn1':
                    checkBounds('gn1', min_gn1, max_gn1);
                    io.Emit('setParameter', tx1_programName, 'tx2', 'gain', ret.gn1);
                    io.Emit('setParameter', rx1_programName, 'rx2', 'gain', ret.gn1);
                    break;
                case 'gn2':
                    checkBounds('gn2', min_gn2, max_gn2);
                    io.Emit('setParameter', tx2_programName, 'tx2_interferer', 'gain', ret.gn2);
                    break;
                case 'bw1':
                    checkBounds('bw1', min_bw1, max_bw1);
                    //parameters.bw1 = 1.0e6*rate/value;
                    console.log("rate=" + rate);
                    io.Emit('setParameter', tx1_programName, 'tx2', 'resampFactor', 1.0e6*rate/ret.bw1);
                    io.Emit('setParameter', rx1_programName, 'rx2', 'resampFactor', ret.bw1/(rate*1.0e6));
                    break;
                case 'bw2':
                    checkBounds('bw2', min_bw2, max_bw2);
                    io.Emit('setParameter', tx2_programName, 'tx2_interferer', 'resampFactor', 1.0e6*rate/ret.bw2);
                    break;
                case 'mcs1':
                    checkBounds('mcs1', min_mcs1, max_mcs1);
                    io.Emit('setParameter', tx1_programName, 'liquidFrame2', 'mode', ret.mcs1);
                    break;
                case 'mcs2':
                    checkBounds('mcs2', min_mcs2, max_mcs2);
                    io.Emit('setParameter', tx2_programName, 'liquidFrame_i', 'mode', ret.mcs2);
                    break;

                default:
                    userData[key] = ret[key];
            }
        });

    }


    editor.on('change', function() {

        if(select.className !== 'edited')
            select.className = 'edited';
        if(running)
            Stop();
    });

    function Start() {
        if(running) return;

        console.log("Starting");
        // Reset userData at each start?
        //userData = false;

        userFunction = new Function(
                    'freq1', 'bw1', 'gn1', 'mcs1', 'bytes1',
                    'freq2', 'bw2', 'gn2', 'mcs2', 'bytes2',
                    'dt', 'userData', 'init', editor.getValue());

        init = true;
        intervalId = setInterval(callUserFunction, Math.round(1000.0*dt)/*milliseconds*/);
        checkbox.checked = true;
        running = true;
    }

    function Stop() {
        console.log("Stopping");
        clearInterval(intervalId);
        checkbox.checked = false;
        running = false;
    }


    checkbox.onclick = function() {
        if(this.checked) {
            if(!running)
                Start();
        } else if(running)
            Stop();
    };


    io.On('getParameter', function(programName,
            controlName, parameter, value) {

        //console.log(controlName + ':' + parameter + "=" + value);

        switch(controlName) {

            case 'tx2': // This is the signal 1 transmitter.
                if(!tx1_programName)
                    tx1_programName = programName;

                switch(parameter) {
                    case 'freq':
                        parameters.freq1 = value;
                        break;
                    case 'gain':
                        parameters.gn1 = value;
                        break;
                    case 'resampFactor': // bw1
                        parameters.bw1 = 1.0e6*rate/value;
                        console.log("parameters.bw1=" + parameters.bw1);
                        break;
                }
                break;
            case 'tx2_interferer': // This is the signal 2 transmitter.

                if(!tx2_programName)
                    tx2_programName = programName;

                switch(parameter) {
                    case 'freq':
                        parameters.freq2 = value;
                        break;
                    case 'gain':
                        parameters.gn2 = value;
                        break;
                    case 'resampFactor': // bw2
                        parameters.bw2 = 1.0e6*rate/value;
                        console.log("parameters.bw2=" + parameters.bw2);
                }
                break;
            case 'rx2':
                if(!rx1_programName)
                    rx1_programName = programName;
            case 'liquidSync':
                if(parameter === 'totalBytesOut')
                    parameters.bytes1 = value;
                break;
            case 'liquidFrame2':
                if(parameter === 'mode')
                    parameters.mcs1 = value;
                break;
            case 'liquidFrame_i':
                if(parameter === 'mode')
                    parameters.mcs2 = value;
                break;
        }
    });
}


// onConnect is defined in session.js and we override it here, so we can
// get the io object after the webSocket connection is made.
// onConnect(io) is called after the webSocket connection is made.
//
var onConnect = function(io) {

        SetupRunUserJavaScript(io);
};
