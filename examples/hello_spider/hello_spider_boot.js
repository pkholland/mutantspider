
document.addEventListener('DOMContentLoaded', function(event) {


    function updateStatus(opt_message)
    {
        var statusField = document.getElementById('statusField');
        if (statusField)
            statusField.innerHTML = opt_message;
    }

    // called by mutantspider.initializeElement when the plugin-element is ready
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

    var hs_elm = document.getElementById('black_dot');
    mutantspider.initialize_element(hs_elm, {name: 'hello_spider', asm_memory: 64*1024*1024}, my_on_ready);

});
