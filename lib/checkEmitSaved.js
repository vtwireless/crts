// This is just little helper functions that let the 'admin' client know
// if the state of the server is saved to a file.  This keeps the server
// from sending 'unsaved' to the admin every time we change the state in
// the server; so the markUnsaved() is if-ed out to doing nothing if the
// server is already marked as 'unsaved'.   This just keeps the code
// smaller by not having 4-5 lines of code of the markUnsaved() function
// in so many other functions in the server code; instead it's just one
// line: checkEmitSaved.markUnsaved(io).
//
// Not much code here.  So small comments do not help.


// It would be a very expensive calculation to determine if the state in
// the "state file" was the same as the server state, so instead of
// parsing the server state file and comparing it to all the savable state
// in the running server, just assume that the two differ each time an
// event happens that could make them differ; by marking this flag,
// isSaved.
//
var isSaved = false;


module.exports = {

    // Called just after saving the state:
    //
    // This is not called as often as markUnsaved().
    markSaved: function(io) {
        isSaved = true;
        io.BroadcastUser('saved', 'admin');
        io.Emit('saved');
    },

    // Called when session just started:
    sendSavedState: function(io) {
        if(isSaved)
            io.Emit('saved');
        else
            io.Emit('unsaved');
    },

    // Called any time there is a server state change.
    markUnsaved: function(io) {
        if(isSaved === false) return;
        isSaved = false;
        io.BroadcastUser('unsaved', 'admin');
        io.Emit('unsaved');
    }
};
