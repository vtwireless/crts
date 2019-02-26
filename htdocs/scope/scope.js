
function fail() {

    // TODO: add stack trace or is browser debugger enough?
    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    const line = '--------------------------------------------------------';
    text += '\n' + line + '\nCALL STACK\n' + line + '\n' +
        new Error().stack + '\n' + line + '\n';
    console.log(text);

    if(crts) crts.cleanup();

    alert(text);
    window.stop();
    throw "javascript error"
}

function assert(val, msg=null) {

    if(!val) {
        if(msg)
            fail(msg);
        else
            fail("JavaScript failed");
    }
}



function _scopeGetGridSpacing(pixPerGrid, min, max, size/*pixels*/) {

    var delta = max - min;

    return { start: 0.0, delta: delta };
}




// @param period time to scroll the whole visual canvas.

function Scope(period) {


    // This class object has 3 canvases, all the same size.
    //
    //  1. render:  the canvas that is showing in the window
    //
    //  2. bg: a canvas (buffer) that we put the constant background on
    //
    //  3. fg: a canvas (buffer) that we rotate through drawing the plot
    //

    var render = document.createElement('canvas');
    render.className = 'render';

    this.getElement = function() {
        return render;
    };


    var xMin = - period, xMax = 0.0;
    var yMin = -0.1, yMax = 1.0;


    var ctx = render.getContext('2d');

    var bg = document.createElement('canvas');
    render.className = 'bg';
    var fg = document.createElement('canvas');
    render.className = 'fg';

    var oldW = -1, oldH = -1, oldSec = -1;

    // Minimum major Grid pixels per grid line.  Larger means the grid
    // lines have more pixels between them.
    var majPixPerGrid = 40;
    var majPixPerGridX = majPixPerGrid; // for vertical grid lines
    var majPixPerGridY = majPixPerGrid; // for horizontal grid lines

    var majGridX = null;
    var majGridY = null;


    function run() {

        var w = canvas.width;
        var h = canvas.height;

        if(canvas.offsetWidth === w && canvas.offsetHeight === h &&
                majGridX !== null) {
            // Nothing new to draw.  We do not draw if there is no change
            // in what we would draw.
            return;
        } else {
            //console.log("w,h=" + w + ',' + h);
            w = canvas.width = canvas.offsetWidth;
            h = canvas.height = canvas.offsetHeight;

            majGridX = _scopeGetGridSpacing(majPixPerGridX, xMin, xMax, w/*pixels*/);
            majGridY = _scopeGetGridSpacing(majPixPerGridY, yMin, yMax, -h/*pixels*/); 


        }

        //ctx.drawImage(fg, 0, 0);

    }

    this.run = run;
}
