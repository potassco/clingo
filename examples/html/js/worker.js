importScripts('clingo.js');

let inputElement = '';
let position = 0;

const messageSchemas = {
    run: {
        args: "array",
        input: "string",
    },
};

function validateMessage(msg, schemas) {
    if (!msg || typeof msg !== 'object') {
        return "Invalid message format: Expected an object.";
    }
    if (!msg.type || typeof msg.type !== 'string') {
        return "Invalid message: 'type' must be a string.";
    }
    const schema = schemas[msg.type];
    if (!schema) {
        return `Unknown message type: '${msg.type}'.`;
    }
    for (const [key, expectedType] of Object.entries(schema)) {
        const actualType = Array.isArray(msg[key]) ? "array" : typeof msg[key];
        if (actualType !== expectedType) {
            return `Invalid '${msg.type}' message: '${key}' must be of type '${expectedType}', but got '${actualType}'.`;
        }
    }
    return null;
}

Module({
    print: (text) => {
        postMessage({ type: "stdout", value: text });
    },
    printErr: (text) => {
        postMessage({ type: "stderr", value: text });
    },
    stdin: () => {
        if (position < inputElement.length) {
            return inputElement.charCodeAt(position++);
        }
        return null;
    },
    monitorRunDependencies: (left) => {
        postMessage({ type: "dependencies", value: left });
    },
}).then(function (Clingo) {
    self.addEventListener('message', (e) => {
        const msg = e.data
        const error = validateMessage(msg, messageSchemas);
        if (error) {
            postMessage({ type: "stderr", value: error });
        }
        else if (msg.type === 'run') {
            const vec = new Clingo.StringVec();
            for (const arg of msg.args) {
                vec.push_back(arg);
            }
            inputElement = msg.input;
            position = 0;
            Clingo.run(vec);
        }
    });
    postMessage({ type: "ready" })
});
