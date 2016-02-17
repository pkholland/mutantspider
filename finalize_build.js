/*
  General Concept: called by 'make' during its first pass to construct and return a string which 'make'
  will evaluate using (gnu) make's 'eval' function.  This string will contain the make targets, dependencies
  and recipies so that when evaluated by make, make will construct all of the files needed to prepare
  for posting a file set to a mutantspider-like AWS S3 bucket.
  
  Note that any text written to console.log (stdout) will be interpreted by make as this "returned"
  string.  Anything written to console.error (stderr) will be piped to whatever stderr make is using.
*/

"use strict"

let fs = require('fs');
let scc = require('strongly-connected-components');
let argv = require('minimist')(process.argv.slice(2));
let ms = require('./mutantspider.js');
let path = require('path');

ms.make_assert(!argv.config_file, 'no --config_file argument supplied');
ms.make_assert(!argv.prep_dir, 'no --prep_dir argument supplied');
ms.make_assert(!argv.build_files, 'no --build_files argument supplied or an empty file list');

let prep_dir = argv.prep_dir + '/';

// read the config_file into a string and call us back with that string
// second parameter is 'first_pass_make' and controls how failed runtime
// assertions are handled.  This is called during the first pass, so set
// the parameter to true
ms.read_file_and_start(argv.config_file, true, (config_str) => {

  // any uncaught exception results in writing
  // the error text to stdout, and then returning
  // a string to make that tells it to stop.
  try {
    let config = JSON.parse(config_str);
      
    // TODO - this should keep sub directory structure beneath argv.build_dir, but currently doesn't!
    let file_list_arr = argv.build_files.split(' ');
    let file_list = [];
    let file_origins = {};
    file_list_arr.forEach((file_name) => {
      let basename = path.basename(file_name);
      if (basename) {
        file_list.push(basename);
        file_origins[basename] = file_name;
      }
    });
    
    // map[<file_name>] == an index
    // reverse_map[<that index>] == <that file>
    let map = {};
    let reverse_map = [];
    file_list.forEach((file_name) => {
      map[file_name] = reverse_map.length;
      reverse_map.push(file_name);
    });
    
    // list of file name extensions that we consider to be "binary" in the sense
    // that we won't scan their contents looking for text patterns.
    let bin_exts = [ '.mem', '.pexe' ];
    
    // for each file in file_list that is a text-like type, check to see with other file(s) in
    // file_list it contains a reference to.  For example, if file_list contains both "index.html"
    // and "app.js", and the contents of index.html contains the string "app.js" then we record
    // the fact that "index.html" contains a reference to (and so is dependent on) "app.js"
    let dependencies = {};
    let is_a_dependent = {};
    file_list.forEach((file_name) => {
      
      if (bin_exts.indexOf(path.extname(file_name)) === -1) {
        (() => {
          let deps = [];
          let f = fs.readFileSync(file_origins[file_name], 'utf8');
          file_list.forEach((other_file_name) => {
            if (file_name !== other_file_name) {
              if ((f.indexOf("'" + other_file_name + "'") !== -1) || (f.indexOf('"' + other_file_name + '"') !== -1)) {
                deps.push(other_file_name);
                is_a_dependent[other_file_name] = true;
              }
            }
          });
          dependencies[file_name] = deps;
        })();
      } else
        dependencies[file_name] = [];
      
    });
    
    // construct the "graph" (strongly-connected-components) with arrays of arrays of integers
    // that represent the files in our set and which ones are dependent on which others.
    let graph = [];
    file_list.forEach((file_name) => {
      let edges = [];
      dependencies[file_name].forEach((dep_name) => {
        edges.push(map[dep_name]);
      });
      graph.push(edges);
    });

    // scc(graph).components will be an array of arrays.  Each inner array element is a single
    // "strongly connected component" of indicies (reverse_map[<that index>]) representing a collection
    // of files that all depend on each other.
    let make_string = '';
    scc(graph).components.forEach((comp) => {
      make_string += make_rule_scc(comp);
    });
    
    // a "top level" component is one that is not referenced by any other component, not
    // in the same strongly connected component.  There is a good chance that these are
    // files like "index.html" that we will want to display on the command line
    make_string += 'ms.top_components:=';
    file_list.forEach((file_name) => {
      if (!is_a_dependent[file_name])
        make_string += ' ' + file_name;
    });
    make_string += '\n\n';
    
    // if we get this far without an exception, then 'make_string' will contain the make snippet
    // for everything we have computed.  So write that to stdout and exit.
    console.log(make_string);
    
    ////////////////////////////
    //
    //    helper functions after this point
    //
    ////////////////////////////
    
    // generate the make snippet for a single file that will be uploaded
    function make_rule_file(file_name) {
    
      // the file in prep_dir is always based on the original, so start with that prerequisite
      let rule = prep_dir + file_name + ': ' + file_origins[file_name];
      
      // it is also dependent on the sha1 files for every file the original contains a reference to
      dependencies[file_name].forEach((dep_name) => {
        rule += ' ' + prep_dir + dep_name + '.sha1';
      });
      
      // and all files list the order-only prerequisite that forces the target directory to be created
      rule += ' | ' + prep_dir + 'dir.stamp\n';
      
      // now, if this target file has dependencies then the recipe for getting it will involve reading the file
      // through 'cat' and piping that to 'sed' to do the replacements.  Otherwise the file is "copied"
      // via 'ln'
      if (dependencies[file_name].length > 0) {
        rule += '\t$(call ms.CALL_TOOL,cat,$(realpath $<)';
        dependencies[file_name].forEach((dep_name) => {
          let url_loc = config.http_loc + ms.root_dir + config.sdk_component;
          rule += ' | $(call ms.do_sed,' + dep_name + ',' + url_loc + ',$(realpath ' + prep_dir + dep_name + '.sha1))';
        });
        rule += ' > $@,$@)\n\n';
      } else
        rule += '\t$(call ms.CALL_TOOL,ln,-f $< $@,$@)\n\n';
        
      // now the rule for the compressed file that we will actually upload
      rule += prep_dir + file_name + '.gz: ' + prep_dir + file_name + '\n';
      rule += '\t$(call ms.CALL_TOOL,gzip,-fnc9 $^ > $@,$@)\n\n';
        
      return rule;
      
    }
    
    // generate the make snippet for a single strongly connected component
    // (a group of files -- possibily only one -- that all contain references to each other)
    function make_rule_scc(strongly_connected_component) {
    
      // get the files for this scc in an array of file names, sorted
      // strongly_connected_component itself is an array of integer
      // indices that reference the files
      let component_files = [];
      strongly_connected_component.forEach((file_index) => {
        component_files.push(reverse_map[file_index]);
      });
      component_files = component_files.sort();
      
      let rules = '';
      
      // each file that will be uploaded is dependent on its original file
      // and the sha1 file for each file the original is dependent on.  That is
      // if (local)index.html contains a reference to "app.js", then
      // (upload)index.html depends on (local)index.html (because it needs to
      // start by copying its contents) and the sha1 value of (local)app.js
      // (because it will replace the "app.js" reference with (url)app.js.<sha1>)
      // Start by emiting the make rules for this part of it.
      component_files.forEach((file_name) => {
        rules += make_rule_file(file_name);
      });
      
      // now emit the make rules for generating the sha1 files themselves - and
      // note that we do this completely differently for strongly_connected_components
      // that have more than one file in them vs. only one file in them.
      
      if (component_files.length > 1) {
      
        // more than one file in the strongly connected component.  Strategy is to
        // compute a checksum value that can uniquely represent the contents of the
        // set of all files in this scc, plus all files that any of these files are
        // are dependent on.  We need a checksum that will change any time any of
        // this files, or any of their dependent children (+ grandchildren, etc...)
        // change, but will always produce the same checksum if starting with this
        // exact set of files (regardless of what machine you execute this code on,
        // or when you execute it).
        //
        // We do this by concatenating the contents of all of these files, plus the
        // sha1 checksums of all of their children, using a repeatable sorting rule,
        // and then compute the sha1 of that.
        
        // start with the file names of all of the dependent children
        let immediate_children = [];
        component_files.forEach((file_name) => {
          dependencies[file_name].forEach((dep_name) => {
            if ((immediate_children.indexOf(dep_name) === -1) && (component_files.indexOf(dep_name) === -1))
              immediate_children.push(dep_name);
          });
        });
        immediate_children = immediate_children.sort();
        
        // now, arbitrarily choose the first file in component_files to make a full
        // dependency rule for generating its sha1 based on all of the data described
        // above.  All other files in this group will specify that their sha1 rule is
        // based on this first one, and just copies its value.
        rules += prep_dir + component_files[0] + '.sha1:';
        component_files.forEach((file_name) => {
          rules += ' ' + file_origins[file_name];
        });
        immediate_children.forEach((file_name) => {
          rules += ' ' + prep_dir + file_name + '.sha1';
        });
        rules += '\n';
        
        // normally we use the "order-only" prerequisite " | <dir>/dir.stamp" to force
        // efficient creation of the destination directory.  But in this case we prefer to
        // not list that prerequisite so that we can use the "$^" make variable to refer
        // to all of the prerequisite.  It is possible that the entire set of files to
        // be posted would be in this single strongly_connected_component, and in that case
        // no rule would have forced the creation of prep_dir, so we explicity force
        // the directory creation here (with -p).
        rules += '\t$(ms.mkdir) -p $(@D)\n';
        rules += '\t$(call ms.CALL_TOOL,cat,$^ | shasum | sed \'s/ .*//\' > $@,$@)\n\n';
        
        // now the rest of them in the group are done by copy
        for (var i = 1, file_name; i < component_files.length; file_name = component_files[i++]) {
          rules += prep_dir + file_name + '.sha1: ' + prep_dir + component_files[0] + '.sha1\n';
          rules += '\t$(call ms.CALL_TOOL,ln,-f $< $@,$@)\n\n';
        }
      
      } else {
      
        // only a single file, strategy is to simply say that file.sha1 is dependent
        // on (only) file, and that the rule to generate file.sha1 is just to call
        // the 'shasum' tool
        let file_name = component_files[0];
        rules += prep_dir + file_name + '.sha1: ' + prep_dir + file_name + '\n';
        rules += '\t$(call ms.CALL_TOOL,shasum,$^ | sed \'s/ .*//\' > $@,$@)\n\n';
      
      }
      
      return rules;
    
    }


  } catch(err) {
    ms.make_assert(err);
  }

});

