Module = require("./parser.js")
Status = {
    preRun: function() { },
    postRun: function() { },
    setStatus: function(text) {
        console.log(text);
    },
    print: function(text) {
        if (arguments.length > 1) {
            text = Array.prototype.slice.call(arguments).join(' ');
        }
        console.log(text);
    },
    printErr: function(text) {
        if (arguments.length > 1) {
            text = Array.prototype.slice.call(arguments).join(' ');
        }
        if (text == "Calling stub instead of signal()") {
            return;
        }
        var prefix = "pre-main prep time: ";
        if (typeof text == "string" && prefix == text.slice(0, prefix.length)) {
            text = "Ready to go!"
        }
        console.log(text);
    },
    totalDependencies: 0,
    monitorRunDependencies: function(left) {
        this.totalDependencies = Math.max(this.totalDependencies, left);
        if (left > 0) {
            this.setStatus('Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')');
        }
    }
}
Status.setStatus("Starting...")
Module(Status).then((instance) => {
    preprocess = instance.cwrap('run', null, ['string', 'number', 'number', 'bool'])
    preprocess("p(X;Y) :- q(X,Y), not p(X,_).", 3, 2, true)
});
