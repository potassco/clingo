const Clingo = (() => {
    const outputElement = document.getElementById('output');
    const inputElement = ace.edit("input");
    const projectMode = document.getElementById('project_mode');
    const appMode = document.getElementById('mode');
    const numModels = document.getElementById('models');
    const projectAnonymous = document.getElementById('project_anonymous');
    const logLevel = document.getElementById('log_level');

    let worker = null;
    let output = "";

    inputElement.setTheme("ace/theme/textmate");
    inputElement.$blockScrolling = Infinity;
    inputElement.setOptions({
        useSoftTabs: true,
        tabSize: 2,
        maxLines: Infinity,
        mode: "ace/mode/clingo",
        autoScrollEditorIntoView: true
    });

    const stripAnsiCodes = (input) =>
        input.replace(/\x1b\[[0-9;]*m/g, '');

    const clearOutput = () => {
        output = ""
        if (outputElement) {
            outputElement.textContent = output;
        }
    }

    const updateOutput = (text) => {
        if (text !== null) {
            output += text + "\n"
        }
        if (outputElement) {
            outputElement.textContent = output;
        }
    }

    const buildArgs = () => {
        let args = []
        args.push('--mode=' + appMode.value)
        args.push('--projection-mode=' + projectMode.value);
        args.push('--log-level=' + logLevel.value);
        args.push('--models=' + numModels.value);
        if (projectAnonymous.checked) {
            args.push('--project-anonymous');
        }
        return args;
    }

    const startWorker = () => {
        if (worker) {
            worker.terminate();
            worker = null;
        }
        const args = buildArgs();
        worker = new Worker('js/worker.js');


        clearOutput()
        updateOutput("Downloading...")
        let n = 0;
        const stdin = inputElement.getValue()
        worker.onmessage = function (e) {
            const msg = e.data
            switch (msg.type) {
                case "dependencies":
                    const i = msg.value
                    n = Math.max(n, i);
                    if (i) {
                        updateOutput(`Preparing... (${n - i}/${n})`)
                    }
                    else {
                        updateOutput('All downloads complete.')
                    }
                    break;
                case "ready":
                    worker.postMessage({ type: 'run', input: stdin, args: args });
                    break;
                case "stdout":
                    updateOutput(msg.value);
                    break;
                case "stderr":
                    updateOutput(stripAnsiCodes(msg.value));
                    break;
            }
        };
    }

    return { 'run': startWorker };
})();
