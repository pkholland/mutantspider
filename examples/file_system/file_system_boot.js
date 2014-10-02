
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
            document.getElementById('test_output').innerHTML += info.message + '<br>';
        else if (info.status == 'error')
            updateStatus('ERROR: ' + info.message);
        else if (info.status == 'crash')
            updateStatus('CRASH: ' + (typeof info.message == 'string' ? info.message : 'plugin experienced an unknown error'));
    }

    // get the 'fs_test' element (in index.html) and tell mutantspider to
    // associate the file_system component with that element.  For this,
    // because we know the file_system code is going to request persistent
    // storage, we tell mutantspider to request it.
    var fs_elm = document.getElementById('fs_test');
    mutantspider.initialize_element(fs_elm, {name: 'file_system', asm_memory: 64*1024*1024, local_storage: 100*1024*1024}, my_on_ready);

});
