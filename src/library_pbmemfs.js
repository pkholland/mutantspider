mergeInto(LibraryManager.library, {
  $PBMEMFS__deps: ['$IDBFS', '$FS', '$MEMFS'],
  $PBMEMFS: {
  
    node_ops: null,
    orig_unlink: null,
    orig_rmdir: null,
    file_stream_ops: null,
    orig_write: null,
    recording_changes: false,

    mount: function(mount) {
      var node = IDBFS.mount.apply(null, arguments);
      if (!PBMEMFS.node_ops) {
        PBMEMFS.node_ops = {};
        for (var p in node.node_ops)
          PBMEMFS.node_ops[p] = node.node_ops[p];
        PBMEMFS.node_ops.mknod = PBMEMFS.mknod;
        PBMEMFS.orig_unlink = node.node_ops.unlink;
        PBMEMFS.node_ops.unlink = PBMEMFS.unlink;
        PBMEMFS.orig_rmdir = node.node_ops.rmdir;
        PBMEMFS.node_ops.rmdir = PBMEMFS.rmdir;
      }
      node.node_ops = PBMEMFS.node_ops;
      return node;
    },

    syncfs: function(mount, populate, callback) {
      IDBFS.syncfs(mount, populate, function(err) {
        if (!err)
          PBMEMFS.recording_changes = true;
        callback(err)
      });
    },

    create_or_delete_node: function(parent, path, create) {
      IDBFS.getDB(parent.mount.mountpoint, function(err, db) {
		
        if (err)
          console.log('IDBFS.getDB(' + parent.mount.mountpoint + ') failed with err: ' + err);
        else {
          var transaction = db.transaction([IDBFS.DB_STORE_NAME], 'readwrite');
          transaction.onerror = function() { console.log('db.transaction([' + IDBFS.DB_STORE_NAME + '], \'readwrite\') failed with err: ' + this.error); };
          var store = transaction.objectStore(IDBFS.DB_STORE_NAME);

          if (create) {
            IDBFS.loadLocalEntry(path, function (err, entry) {
              if (err)
                console.log('IDBFS.loadLocalEntry(' + path + ') failed with err: ' + err);
              else
                IDBFS.storeRemoteEntry(store, path, entry, function(err) {
                  if (err)
                    console.log('IDBFS.storeRemoteEntry(' + path + ') failed with err: ' + err);
                });
            });

          } else {
            IDBFS.removeRemoteEntry(store, path, function(err) {
              if(err)
                console.log('IDBFS.removeRemoteEntry(' + path + ') failed with err: ' + err);
            });
          }
        }
        
      });
    },

    mknod: function(parent, name, mode, dev) {
      var node = MEMFS.createNode(parent, name, mode, dev);
      if (FS.isFile(node.mode)) {
        if (PBMEMFS.recording_changes)
          PBMEMFS.create_or_delete_node(parent, FS.getPath(node), true);
        if (!PBMEMFS.file_stream_ops) {
          PBMEMFS.file_stream_ops = {};
          for (var p in node.stream_ops)
            PBMEMFS.file_stream_ops[p] = node.stream_ops[p];
          PBMEMFS.file_stream_ops.close = PBMEMFS.close;
          PBMEMFS.orig_write = node.stream_ops.write;
          PBMEMFS.file_stream_ops.write = PBMEMFS.write;
        }
        node.stream_ops = PBMEMFS.file_stream_ops;
      } else if (FS.isDir(node.mode)) {
        if (PBMEMFS.recording_changes)
          PBMEMFS.create_or_delete_node(parent, FS.getPath(node), true);
        node.node_ops = PBMEMFS.node_ops;
      }
      return node;
    },

    rmdir: function(parent, name) {
      var path = FS.getPath(FS.lookupNode(parent,name));
      PBMEMFS.orig_rmdir(parent,name);
      PBMEMFS.create_or_delete_node(parent, path, false);
    },

    unlink: function(parent, name) {
      var path = FS.getPath(FS.lookupNode(parent,name));
      PBMEMFS.orig_unlink(parent,name);
      PBMEMFS.create_or_delete_node(parent, path, false);
    },

    write: function(stream, buffer, offset, length, position, canOwn) {
      var bytesWritten = PBMEMFS.orig_write(stream, buffer, offset, length, position, canOwn);
      if (PBMEMFS.recording_changes && (bytesWritten > 0))
        stream.is_dirty = true;
      return bytesWritten;
    },
	
    close: function(stream) {
      if (stream.is_dirty) {
        var lookup = FS.lookupPath(stream.path, { parent: true });
        var parent = lookup.node;
        IDBFS.getDB(parent.mount.mountpoint, function(err, db) {

          if (err)
            console.log('IDBFS.getDB(' + parent.mount.mountpoint + ') failed with err: ' + err);
          else {
            var transaction = db.transaction([IDBFS.DB_STORE_NAME], 'readwrite');
            transaction.onerror = function() { console.log('db.transaction([' + IDBFS.DB_STORE_NAME + '], \'readwrite\') failed with error: ' + this.error); };
            var store = transaction.objectStore(IDBFS.DB_STORE_NAME);
		  
            IDBFS.loadLocalEntry(stream.path, function (err, entry) {
              if (err)
                console.log('IDBFS.loadLocalEntry(' + stream.path + ') failed with err: ' + err);
              else
                IDBFS.storeRemoteEntry(store, stream.path, entry, function(err) {
                  if (err)
                    console.log('IDBFS.storeRemoteEntry(' + stream.path + ') failed with err: ' + err);
                });
            });
          }

        });
      }
    }
    
  }
});
