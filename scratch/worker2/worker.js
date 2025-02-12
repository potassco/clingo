importScripts('hello.js');

let stdout = '';

Module({
    print: function (text) {
        stdout += text + "\n";
    },
    printErr: function (text) {
        console.error(text);
    }
}).then(function (Module) {
    const helloWorld = Module.cwrap('hello_world', 'string', []);
    const result = helloWorld();
    postMessage({ result: result, stdout: stdout });
});
