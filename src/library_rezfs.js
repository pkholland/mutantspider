mergeInto(LibraryManager.library, {
  $REZFS__deps: ['$ERRNO_CODES', '$FS', '$MEMFS'],
  $REZFS: {
  
    ops_table: null,
    
    mount: function(mount) {
      if (!REZFS.ops_table) {
        REZFS.ops_table = {
          dir: {
            node: {
              getattr: REZFS.node_ops.getattr,
              lookup: MEMFS.node_ops.lookup,
              readdir: REZFS.node_ops.readdir,
              mknod: REZFS.node_ops.mknod,
            },
            stream: {
            },
          },
          file: {
            node: {
              getattr: REZFS.node_ops.getattr,
              setattr: REZFS.node_ops.setattr,
            },
            stream: {
              llseek: MEMFS.stream_ops.llseek,
              read: REZFS.stream_ops.read,
            }
          },
        };
      }
      
      var node = FS.createNode(null,'/',{{{ cDefine('S_IFDIR') }}} | 511 /* 0777 */, 0);
    
      node.node_ops = REZFS.ops_table.dir.node;
      node.stream_ops = REZFS.ops_table.dir.stream; // currently empty (see dir.stream above), but FS needs a non-null stream_ops.
      node.contents = ['.', '..'];
      
      REZFS.recursive_mount_dir(mount.opts.root_addr, node);
      
      return node;
    },
    
    node_ops: {
    
      getattr: function(node) {
        var attr = {};
        attr.dev = 1;
        attr.ino = node.id;
        attr.mode = node.mode;
        attr.nlink = 1;
        attr.uid = 0;
        attr.gid = 0;
        attr.rdev = node.rdev;
        if (FS.isDir(node.mode)) {
          attr.size = 4096;
        } else /*if (FS.isFile(node.mode))*/ {
          attr.size = {{{ makeGetValue('node.contents', '4', 'i32') }}};  // <- this line is why we need a custom getattr
        }
        attr.atime = new Date(node.timestamp);
        attr.mtime = new Date(node.timestamp);
        attr.ctime = new Date(node.timestamp);
        attr.blksize = 4096;
        attr.blocks = Math.ceil(attr.size / attr.blksize);
        return attr;
      },
      readdir: function(node) {
        return node.contents;
      },
      setattr: function(node, attr) {
        throw new FS.ErrnoError(ERRNO_CODES.EROFS);
      },
      mknod: function(parent, name, mode, dev) {
        throw new FS.ErrnoError(ERRNO_CODES.EROFS);
      }
    
    },
    
    stream_ops: {
      
      read: function(stream, buffer, offset, length, position) {
        var contents = stream.node.contents;
        var ptr = {{{ makeGetValue('contents', '0', 'i32') }}};
        var len = {{{ makeGetValue('contents', '4', 'i32') }}};
        if (position >= len)
          return 0;
        var size = length;
        if (position + size > len)
          size = len - position;
#if USE_TYPED_ARRAYS == 2
        if (size > 8)
          buffer.set(HEAP8.subarray(ptr+position, ptr+position+size), offset);
        else
#endif
        {
          for (var i = 0; i < size; i++)
            buffer[offset + i] = HEAP8[ptr + position + i];
        }
        return size;
      },
    
    },
    
    recursive_mount_dir: function (dir_addr, parent)  {
      var num_ents = {{{ makeGetValue('dir_addr', '0', 'i32') }}};
      var ents_addr = {{{ makeGetValue('dir_addr', '4', 'i32') }}};
      
      for (var i = 0; i < num_ents; i++) {
        var d_name_addr = {{{ makeGetValue('ents_addr', 'i*12', 'i32') }}};
        var d_name = Pointer_stringify(d_name_addr);
        var ptr = {{{ makeGetValue('ents_addr', 'i*12+4', 'i32') }}};
        var is_dir = {{{ makeGetValue('ents_addr', 'i*12+8', 'i32') }}};
        
        var mode;
        if (is_dir != 0)
          mode = {{{ cDefine('S_IFDIR') }}} | 511/*0777*/;
        else
          mode = {{{ cDefine('S_IFREG') }}} | 292/*0444*/;
        var node = FS.createNode(parent, d_name, mode, 0);
        if (is_dir != 0)
        {
          node.node_ops = REZFS.ops_table.dir.node;
          node.stream_ops = REZFS.ops_table.dir.stream;
          node.contents = ['.', '..'];
          REZFS.recursive_mount_dir(ptr, node);
        }
        else
        {
          node.node_ops = REZFS.ops_table.file.node;
          node.stream_ops = REZFS.ops_table.file.stream;
          node.contents = ptr;
        }
        parent.contents.push(d_name);
        
      }
      MEMFS.node_ops.setattr(parent, {mode: {{{ cDefine('S_IFDIR') }}} | 365/*0555*/});
      parent.is_readonly_fs = true;
    }
  
  }
});
