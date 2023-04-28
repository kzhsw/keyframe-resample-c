export function addRowToTable(tableData) {
    // get the table element
    const table = document.querySelector("table");

    // create a new row
    const newRow = document.createElement("tr");

    // create cells and set their values
    const typeCell = document.createElement("td");
    typeCell.textContent = tableData.type;
    newRow.appendChild(typeCell);

    const memorySizeCell = document.createElement("td");
    memorySizeCell.textContent = tableData.memorySize;
    newRow.appendChild(memorySizeCell);

    const timeEscapedCell = document.createElement("td");
    timeEscapedCell.textContent = tableData.timeEscaped;
    newRow.appendChild(timeEscapedCell);

    const beforeLengthCell = document.createElement("td");
    beforeLengthCell.textContent = tableData.beforeLength;
    newRow.appendChild(beforeLengthCell);

    const beforeFramesCell = document.createElement("td");
    beforeFramesCell.textContent = tableData.beforeFrames;
    newRow.appendChild(beforeFramesCell);

    const afterLengthCell = document.createElement("td");
    afterLengthCell.textContent = tableData.afterLength;
    newRow.appendChild(afterLengthCell);

    const afterFramesCell = document.createElement("td");
    afterFramesCell.textContent = tableData.afterFrames;
    newRow.appendChild(afterFramesCell);

    const frameThroughputCell = document.createElement("td");
    frameThroughputCell.textContent = tableData.frameThroughput;
    newRow.appendChild(frameThroughputCell);

    const byteThroughputCell = document.createElement("td");
    byteThroughputCell.textContent = tableData.byteThroughput;
    newRow.appendChild(byteThroughputCell);

    const ratioCell = document.createElement("td");
    ratioCell.textContent = tableData.ratio;
    newRow.appendChild(ratioCell);

    // add the new row to the table body
    table.querySelector("tbody").appendChild(newRow);
}
