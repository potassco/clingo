importScripts('hello.js');

Module.onRuntimeInitialized = function () {
    const helloWorld = Module.cwrap('hello_world', 'string', []);
    const result = helloWorld();
    postMessage(result);
};
