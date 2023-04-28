import {addRowToTable} from "./table.js";

let suites = [
    {type: 'wasm', memorySize: 0},
    {type: 'wasm', memorySize: 1},
    {type: 'wasm', memorySize: 3},
    {type: 'wasm', memorySize: 7},
    {type: 'simd', memorySize: 0},
    {type: 'simd', memorySize: 1},
    {type: 'simd', memorySize: 3},
    {type: 'simd', memorySize: 7},
    {type: 'js'},
    {type: 'jsOpt'},
];

function formatFileSize(value) {
    let i, byteUnits;
    if (!value) {
        return '';
    }
    i = 0;
    byteUnits = ['', 'kB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB'];
    while (value >= 1024 && byteUnits[i + 1]) {
        value /= 1024;
        i++;
    }
    return (i > 0 ? value.toFixed(2) : value) + byteUnits[i];
}

async function benchmark(warmup, run) {
    let result = [];
    let firstResult = 0;

    for (let suite of suites) {
        let status = document.getElementById('status');
        status.innerText = ' Working, current: ' + suite.type;
        if ('memorySize' in suite) {
            status.innerText += ', wasm memory size: ' +
                formatFileSize((suite.memorySize + 1) * 64 * 1024);
        }
        let worker = new Worker('./benchmark-worker.js', {
            type: "module",
        });
        let ev = await new Promise((resolve, reject) => {
            worker.onmessage = resolve;
            worker.onmessageerror = worker.onerror = (e) => {
                resolve({
                    type: 'error',
                    e,
                });
            };
            worker.postMessage({...suite, warmup, run});
        });
        if (ev.data) {
            let currResult = {
                ...suite,
                ...ev.data,
            };
            if (currResult.err) {
                currResult.timeEscaped = 'Error ' + currResult.err;
            } else {
                if (!firstResult) {
                    currResult.ratio = '1.00';
                    firstResult = currResult.timeEscaped;
                } else {
                    currResult.ratio = (currResult.timeEscaped / firstResult).toFixed(2);
                }
                if ('memorySize' in currResult) {
                    currResult.memorySize = formatFileSize((currResult.memorySize + 1) * 64 * 1024);
                }
                currResult.frameThroughput =
                    Math.round(currResult.beforeFrames / (currResult.timeEscaped / 1000)) + '/s';
                currResult.byteThroughput =
                    formatFileSize(currResult.beforeLength / (currResult.timeEscaped / 1000)) + '/s';
                currResult.timeEscaped = currResult.timeEscaped.toFixed(2);
                result.push(currResult);
            }
            addRowToTable(currResult);
        } else {
            let currResult = {
                ...suite,
                timeEscaped: 'Error ' + ev.toString(),
            };
            currResult.timeEscaped = currResult.timeEscaped.slice(0, 32);
            if ('memorySize' in currResult) {
                currResult.memorySize = formatFileSize((currResult.memorySize + 1) * 64 * 1024);
            }
            result.push(currResult);
            addRowToTable(currResult);
            console.error(suite, ev);
        }
        console.log(suite, ev.data || ev);
        await new Promise(resolve => setTimeout(resolve, 100));
        worker.terminate();
        await new Promise(resolve => setTimeout(resolve, 100));
    }
    document.getElementById('status').innerText = ' Done.';
    console.log(result);
}

document.getElementById('benchmark').onclick = (ev) => {
    const warmup = Number(document.getElementById('warmup').value) || 5;
    const run = Number(document.getElementById('run').value) || 5;
    ev.target.style.display = 'none';
    benchmark(warmup, run).finally(() => {
        ev.target.style.display = '';
    });
};
