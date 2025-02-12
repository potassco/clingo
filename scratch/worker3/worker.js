importScripts('hello.js');

let stdin = '';
let stdinPosition = 0;

Module({
    print: function (text) {
        postMessage({ stdout: text + "\n" });
    },
    printErr: function (text) {
        console.error(text);
    },
    stdin: function () {
        if (stdinPosition < stdin.length) {
            return stdin.charCodeAt(stdinPosition++);
        } else {
            return null;
        }
    }
}).then(function (Module) {
    self.addEventListener('message', function (e) {
        if (e.data.type === 'process') {
            stdin = e.data.input;
            stdinPosition = 0;
            Module.ccall('process_input', null, [], []);
        }
    });
});
