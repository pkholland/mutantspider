
document.addEventListener('DOMContentLoaded', function(event) {

    // set the text of the 'statusField' element (in index.html) to the given message
    function updateStatus(message)
    {
        document.getElementById('statusField').innerHTML = message;
    }

    // called by mutantspider.initializeElement when the plugin-element is ready, as
    // we as several other interesting points
    function my_on_ready(info)
    {
        if (info.status == 'loading')
            updateStatus('LOADING ' + info.exe_type + '...');
        else if (info.status == 'running')
            updateStatus('RUNNING ' + info.exe_type);
        else if (info.status == 'message')
            console.log(info.message);
        else if (info.status == 'error')
            updateStatus('ERROR: ' + info.message);
        else if (info.status == 'crash')
            updateStatus('CRASH: ' + info.message);
    }

    // get the 'black_dot' element (in index.html) and tell mutantspider to
    // associate the hello_spider component with that element
    var bd_elm = document.getElementById('black_dot');
    mutantspider.initialize_element(bd_elm, {name: 'hello_spider', asm_memory: 64*1024*1024}, my_on_ready);

});
