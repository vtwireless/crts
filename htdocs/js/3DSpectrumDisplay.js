var got_threeDSpectrumDisplay = false;


function threeDSpectrumDisplay(tag) {

    if(got_threeDSpectrumDisplay)
        // We make just one.
        return;

    got_threeDSpectrumDisplay = true;

    var tSteps = 10; // number of time values to display
    var bins = 0;
    var heights = '';
    var lastHeightsLength = 0;
    var colors = '';
    var elevationGrid = GetElementById('elevationGrid');
    var color = GetElementById('color');
    var maxHeight = -9.9e+15, minHeight = 9.9e+15;
    
    function threeDSpectrumDisplay_set(
            freq, bandwidth, _bins, updateRate) {

        spew('threeDSpectrumDisplay_set(' +
            'freq=' + freq +
            ', bandwidth=' + bandwidth +
            ', bins=' + _bins +
            ', updateRate=' + updateRate +
            ')');

        bins = _bins;
        heights = '';
        colors = '';
        lastHeightsLength = 2*bins;
        for(z=0;z<tSteps; ++z) {
            for(let x=0; x<bins; ++x) {
                heights += '0 ';
                colors += '0.2 1 0.2 ';
            }
        }

        //elevationGrid.setAttribute('height', heights);
        //color.setAttribute('color', colors);
    }

    function threeDSpectrumDisplay_update(values) {

        spew('threeDSpectrumDisplay_update(' + values + ')');


        /*
        assert(values.length == bins,
            "threeDSpectrumDisplay_update([" + values + "])");

        heights = heights.substr(lastHeightsLength);
        lastHeightsLength = 0;

        var maxHeight = -9.9e+15, minHeight = 9.9e+15;
        for(let i=0; i<bins; ++i) {
            if(values[i] > maxHeight)
                maxHeight = values[i];
            if(values[i] < minHeight)
                minHeight = values[i];
        }

        spew('max=' + maxHeight + ' min=' + minHeight);

        for(let i=0; i<bins; ++i) {
            let val = (values[i] - minHeight)/(maxHeight - minHeight);
            let str = val.toString() + ' ';
            lastHeightsLength += str.length;
            heights += str;
        }

        spew('heights=' + heights);

        //elevationGrid.setAttribute('height', heights);
        */
    }

    function threeDSpectrumDisplay_destroy() {

        spew('threeDSpectrumDisplay_destroy()');
        got_threeDSpectrumDisplay = false;
    }

    SpectrumDisplay_create(tag,
        threeDSpectrumDisplay_update,
        threeDSpectrumDisplay_set,
        threeDSpectrumDisplay_destroy);
}
