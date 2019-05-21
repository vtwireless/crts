
require('/scope/scope.js');
require('/scope/scope.css');



// This function creates a WDApp that has a 2D scope view of the
// spectrum data that is passed in its' draw function.
//
// Interfaces:
//
//    ScopeDisplay() constructor function
//
//       and
//
//    ScopeDisplay is also a static methods object like Array
//      (TODO)
//
//
//   static methods:
//
//
//
//   instance methods:
//
//      this->draw(x, y, cFreq, bandwidth, updatePeriod)
//
//        called by thing using this object when it has
//        new data.
//
//
//      this->destroy()
//
//        called by thing using this object when it done
//        with this scope thingy.
//
//

// This is how to make a function named "2D Scope".
//
window['2D Scope'] = function(desc, onclose = null) {

    var ampMax, ampMin;
    var cFreq = 0, bandwidth = 0, bins = 0;

    var scope = new Scope;

    var app = new WDApp(desc,
            scope.getElement(),
            function() {
                console.log('Removing spectrum scope "' +
                    desc + '"');
                if(onclose) onclose();
            app = null;
    });

    console.log('Made 2D Scope: ' + desc);

    this.destroy = function() {
        if(app) app.close();
        app = null;
    };

    this.draw = function(x, y, _cFreq, _bandwidth, _updatePeriod) {

        // We have new spectrum data.
        let l = x.length;

        if(cFreq !== _cFreq || bandwidth !== _bandwidth || l !== bins) {

            // The parameters have changed since the last draw call.
            cFreq = _cFreq;
            bandwidth = _bandwidth;
            bins = l;

            // Delete the old ampM* arrays.
            ampMax = [];
            ampMin = [];

            for(let i=0; i<l; ++i) {
                // Initialize the arrays.
                ampMax[i] = Number.MIN_VALUE;
                ampMin[i] = Number.MAX_VALUE;
            }
        }

        for(let i=0; i<l; ++i) {
            if(y[i] > ampMax[i])
                ampMax[i] = y[i];
            if(y[i] < ampMin[i])
                ampMin[i] = y[i];
        }

        scope.draw(x, y, 0);
        scope.draw(x, ampMax, 1);
        scope.draw(x, ampMin, 2);
        scope.draw();
    };
};
