// This javaScript is paired with the file ../3DSpectrumDisplay.htm
// which has the x3d xml scene graph in it, this also needs the
// x3dom javaScript, x3dom.js, and the crts.js loaded.


// One stink'n global to flag the singleton existence
// in addition to the constructor threeDSpectrumDisplay().
var threeDSpectrumDisplay_obj = null;


// Create a x3DOM 3D canvas and use SpectrumDisplay_create() to set
// callbacks to feed it data.
function threeDSpectrumDisplay(feedTag, parentNode=null, _nSteps = 3) {

    if(threeDSpectrumDisplay_obj)
        // We make just one 3DSpectrumDisplay.  We tried to make more
        // but x3dom bugs totally stopped us.
        //
        // TODO: At least for now, but we could extend to any number
        // of these.
        return threeDSpectrumDisplay_obj;


    let nSteps = _nSteps, // number of values to display at a time
        bins, // number of values per time step
        freq, // center frequency
        values, // values[step][bin]
        lastHeightsLength = 0,
        // x3dom elevationGrid that is in 3DSpectrumDisplay.htm
        elevationGrid = null,
        // color in the x3dom elevationGrid
        color = null,
        // to scale the values
        yScale = 1.0e3, // height scale
        maxHeight = -9.9e+15, minHeight = 9.9e+15,
        running = false;


    var obj = threeDSpectrumDisplay_obj = {};

    obj.setNumTimeSteps = function(_nSteps) {
        nSteps = _nSteps;
    };


    function OnLoad() {

        // We can't set elevationGrid and color until
        // 3DSpectrumDisplay.htm is loaded

        // x3dom elevationGrid that is in 3DSpectrumDisplay.htm
        elevationGrid = GetElementById('elevationGrid');

        // color in the x3dom elevationGrid
        color = GetElementById('color');


        function setHeightAndColor() {

            let heights = '';
            colors = '';
            for(z=0;z<nSteps; ++z) {
                for(let x=0; x<bins; ++x) {
                    heights += values[z][x].toString() + ' ';
                    colors += '0.2 1 0.2   ';
                }
            }
            // Add a zero height row to the front.
            for(let x=0; x<bins; ++x) {
                heights += '0 ';
                colors += '1 0.2 0.2 ';
            }

            elevationGrid.setAttribute('height', heights);
            color.setAttribute('color', colors);

            //spew('elevationGrid.outerHTML=' + elevationGrid.outerHTML);
            
            // Reload the x3dom elevationGrid to force it to render
            // our changes.
            var container = elevationGrid.parentNode;

            assert(container, "container is false");

            // This is super stupid.  What a waste.  We reload the
            // elevationGrid in order to get it to render.
            //
            // TODO: tell X3dom to rerender the elevationGrid without
            // reloading it.

	    var content = container.innerHTML;
	    container.innerHTML = content;
        
            // x3dom elevationGrid that is in 3DSpectrumDisplay.htm
            elevationGrid = GetElementById('elevationGrid');

            // color in the x3dom elevationGrid
            color = GetElementById('color');

        }


        // This is called when parameters change.
        function threeDSpectrumDisplay_set(
                _freq, _bandwidth, _bins, _updateRate) {

            freq = _freq;
            bandwidth = _bandwidth;
            bins = _bins;
            updateRate = _updateRate;

            dspew('threeDSpectrumDisplay_set(' +
                'freq=' + freq +
                ', bandwidth=' + bandwidth +
                ', bins=' + bins +
                ', updateRate=' + updateRate +
                ')');

            values = []; // values[nStep][bins]
            // Example: values[2][20]  row 2   bin slot 20

            for(let z=0; z<nSteps; ++z) {
                let vals = [];
                values[z] = vals;
                for(let x=0; x<bins; ++x)
                    vals[x] = 0.5;
            }

            elevationGrid.setAttribute('xDimension', bins.toString());
            elevationGrid.setAttribute('zDimension', (nSteps + 1).toString());
            elevationGrid.setAttribute('xspacing', 3.0/(nSteps + 1));
            elevationGrid.setAttribute('zspacing', 3.0/bins);

            setHeightAndColor();
         }

        function threeDSpectrumDisplay_update(_values) {

            //spew('threeDSpectrumDisplay_update(' + _values + ')');

            assert(_values.length == bins,
                "threeDSpectrumDisplay_update([" + _values +
                "]) with bins=" + bins);

            for(let z=0; z<nSteps-1; ++z)
                for(let x=0; x<bins; ++x)
                    values[z][x] = values[z+1][x];

            for(let x=0; x<bins; ++x)
                values[nSteps-1][x] = _values[x] * yScale;

            setHeightAndColor();
        }

        function threeDSpectrumDisplay_destroy() {

            spew('threeDSpectrumDisplay_destroy()');
            got_threeDSpectrumDisplay = false;
        }

        SpectrumDisplay_create(feedTag,
            threeDSpectrumDisplay_update,
            threeDSpectrumDisplay_set,
            threeDSpectrumDisplay_destroy);

    
        obj.setNumTimeSteps = function(_nSteps) {
            nSteps = _nSteps;
            if(running)
                threeDSpectrumDisplay_set(freq, bandwidth, bins, updateRate);
        };

        spew("3DSpectrumDisplay is OKAY");
    }


    if(parentNode === null) {

        // We must have the x3d html loaded already
        // so we proceed to hook it up.
        OnLoad();

    } else {

        // We have a request to load a new <x3d> and then
        // hook it up with the callbacks.
        let xhttp = new XMLHttpRequest();
        let url = '../3DSpectrumDisplay.htm';
        xhttp.open("GET", url);
        xhttp.onreadystatechange = function() {
            
            if(this.readyState == 4 && this.status == 200) {

                let x3d = document.createElement('x3d');
                x3d.innerHTML = this.responseText;
                parentNode.appendChild(x3d);
                if(typeof(x3dom) === 'undefined') {

                    // TODO: x3dom.js cannot be loaded dynamically in
                    // javaScript due to a bug in it.  But were it not
                    // for this bug the following could be used to
                    // load x3dom.js on the fly.

                    assert("The page did not include x3dom.js");

                    var head= document.getElementsByTagName('head')[0];
                    let script = document.createElement('SCRIPT');
                    script.onload = function() {
                        // proceed to hook up this newly loaded X3D node.
                        OnLoad();
                    };
                    head.appendChild(script);
                    script.src = "../x3dom/x3dom.js";
                } else {
                    // The X3DOM javaScript has been loaded already.
                    spew('\n\ncalling x3dom.reload()\n\n');

                    x3dom.runtime.ready = function() {
                        OnLoad();
                    };

                    x3dom.reload();
                }
            } else {
                dspew('GET ' + url + ' returned status ' + this.status);
            }
        };
        xhttp.send();
    }

    return obj;
}
