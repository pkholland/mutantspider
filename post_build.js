/*
  General Concept: called by 'make' to upload a set of files to an s3 bucket.
  
*/

"use strict"

let fs = require('fs');
let aws = require('aws-sdk');
let argv = require('minimist')(process.argv.slice(2));
let ms = require('./mutantspider.js');
let path = require('path');

ms.make_assert(!argv.config_file, 'no --config_file argument supplied');
ms.make_assert(!argv.prep_dir, 'no --prep_dir argument supplied');
ms.make_assert(!argv.build_files, 'no --build_files argument supplied or an empty file list');

let prep_dir = argv.prep_dir + '/';

function verbose_log(str) {
  if (argv.verbose)
    console.log(str);
}

// read the config_file into a string and call us back with that string
// second parameter is 'first_pass_make' and controls how failed runtime
// assertions are handled.  This is called as a recipe for a target
// (not the "first pass")
ms.read_file_and_start(argv.config_file, false, (config_str) => {

  // any uncaught exception results in writing
  // the error text to stdout, and then exiting the
  // process with a non-zero exit code
  try {
    let config = JSON.parse(config_str);
    
    // validate and configure the aws access to use credentials etc... that are
    // specified in the config file
    ms.assert(!config.aws_region, 'configuration missing required aws_region entry');
    aws.config.update({region: config.aws_region});
    ms.assert(!config.aws_profile, 'configuration missing required aws_profile entry');
    aws.config.credentials = new aws.SharedIniFileCredentials({profile: config.aws_profile});
    ms.assert(!aws.config.credentials.accessKeyId,'profile: \'' + config.aws_profile + '\' missing from your ~/.aws/credentials file');

    // the object that lets us talk to aws's s3 service
    let s3 = new aws.S3();
    
    // TODO - this should keep sub directory structure beneath argv.build_dir, but currently doesn't!
    let file_list_arr = argv.build_files.split(' ');
    if (argv.module_name != '')
      file_list_arr.push(argv.module_name);
    let file_list = [];
    file_list_arr.forEach((file_name) => {
      let basename = path.basename(file_name);
      
      // if build_files has multiple spaces separating files those will result
      // in an empty "basename", ignore those
      if (basename)
        file_list.push(basename);
    });
    
    // the set of files which are not reference by other files
    let top_files = [];
    argv.top_files.split(' ').forEach((file_name) => {
      let basename = path.basename(file_name);
      if (basename)
        top_files.push(basename);
    });

    // the list of all sub directories we are interested in
    // on the server - currently only the main one for this "component"
    let deploy_dirs = [ ms.root_dir + config.sdk_component ];
    
    // given a file name as enumerated by s3, check whether it is
    // in one of the directories we are interested in or not.  If it
    // is, then return a slightly reformated version of it
    function in_tracked_dir(file_name) {
      let ret = null;
      deploy_dirs.forEach((dname) => {
        if (file_name.indexOf(dname) === 0)
          ret = file_name.split(ms.root_dir)[1];
      });
      return ret;
    }

    // when srv_files_prom is resolved, srv_files will be the set
    // of files currently in s3 in any of the directories we
    // are interested in, storing the size of that file keyed
    // by the file name
    let srv_files = {};
    
    // s3 returns in in batches, so fetch the next batch
    function next_fetch(resolve, reject, start_marker) {
      s3.listObjects({Bucket:config.s3_bucket, Marker:start_marker}, (error, data) => {
        if (error)
          reject(error);
        else if (data.Contents.length === 0)
          resolve();
        else {
          data.Contents.forEach((item) => {
            let fname = in_tracked_dir(item.Key);
            if (fname)
              srv_files[fname] = {size:item.Size};
          });
          next_fetch(resolve, reject, data.Contents[data.Contents.length-1].Key)
        }
      });
    }
    
    let srv_files_prom = new Promise((resolve, reject) => {
      next_fetch(resolve, reject);
    });
    
    // a dictionary, keyed by the file name of the files we are asked
    // to upload to s3.  It contains the sha1 hash of the file (prior to
    // gzip compression). Note that both the gzip and sha1 computations are
    // performed prior to this script being called, and the results are stored
    // in files, so the operation simply reads information about those companion
    // files
    let loc_hashes = {};
    
    // add the entry in loc_hashes for 'file_name'
    function read_file(file_name) {
      let sha1_name = prep_dir + file_name + '.sha1';
      let sha1_fd = fs.openSync(sha1_name, 'r');
      let sha1_size = fs.fstatSync(sha1_fd).size;
      let sha1_buffer = new Buffer(sha1_size);
      let bytes_read = fs.readSync(sha1_fd,sha1_buffer,0,sha1_size);
      fs.closeSync(sha1_fd);
      ms.assert(bytes_read !== sha1_size, 'incomplete read of file: ' + sha1_name + ', only read ' + bytes_read + ' bytes of ' + sha1_size);
      loc_hashes[file_name] = sha1_buffer.toString().split('\n')[0];
    }
    
    // Promise that loops through all files in file_list and reads the info about them
    let loc_files_prom = new Promise((resolve, reject) => {
      try {
        file_list.forEach((file_name) => {
          read_file(file_name);
        });
        resolve();
      } catch (err) {
        reject(err);
      }
    });
        
    // when information about all local and server files have been read...
    Promise.all([srv_files_prom, loc_files_prom]).then(() => {
      // at this point srv_files, file_list, and loc_hashes have all been filled out
      
      let ext_map = {
        js:'application/javascript',
        html:'text/html',
        txt:'text/plain',
        css:'text/css'
      };
      
      // compute the contentType based on the file name extension
      function get_contentType(file_name) {
        let contentType = path.extname(file_name);
        if (contentType && contentType[0] === '.')
          contentType = ext_map[contentType.split('.')[1]];
        return contentType || 'application/octet-stream';
      }
      
      // figure out which files we have to upload (we don't upload a file if a match
      // is already present).  Create a Promise for each file upload so we can wait
      // on all of them to complete
      let uploads = [];
      file_list.forEach((file_name) => {
        let sha1 = loc_hashes[file_name];
        let remote_file_name = config.sdk_component + '/' + file_name + '.' + sha1;
        if (srv_files[remote_file_name])
          verbose_log('skipping upload, server already contains matching file for: ' + file_name + '.' + sha1);
        else {
          uploads.push(new Promise((resolve, reject) => {

            let is_module = file_name.includes('-bind.js.mod');
            
            let params = {
              Bucket:config.s3_bucket,
              Key:ms.root_dir + remote_file_name,
              Body:fs.createReadStream(prep_dir + file_name + (is_module ? '' : '.gz')),
              ACL:'public-read',
              ContentType:get_contentType(file_name),
              ContentEncoding:is_module ? 'identity' : 'gzip'
            }
            console.log('uploading         ' + ms.root_dir + remote_file_name);
            s3.upload(params, (error, data) => {
              if (error)
                reject(error);
              else
                resolve();
            });
            
          }));
        }
      });
      
      // if we are labeling the build, then also set all of the redirect files
      if (argv.build_label) {
        top_files.forEach((file_name) => {
          uploads.push(new Promise((resolve, reject) => {
          
            let label_ext = argv.build_label === '.' ? '' : '.' + argv.build_label;
            let nm = ms.root_dir + config.sdk_component + '/' + file_name;
            var params = {
              Bucket:config.s3_bucket,
              Key:nm + label_ext,
              Body:new Buffer(0),
              ACL:'public-read',
              WebsiteRedirectLocation:'/' + nm + '.' + loc_hashes[file_name]
            };
            s3.upload(params, function(error, data) {
              if (error)
                reject(error);
              else
                resolve();
            });

          }));
        });
      }
      
      // wait until all uploads have completed
      Promise.all(uploads).then(() => {
        
        // the promise to update the database enty
        var db_prom = new Promise((resolve, reject) => {
          
          // add the entry to the database about this set files that we just posted (or would have posted
          // if the matches weren't already present)
          let dependent_files = [];
          file_list.forEach((file_name) => {
            let sha1 = loc_hashes[file_name];
            dependent_files.push(ms.root_dir + config.sdk_component + '/' + file_name + '.' + sha1);
          });
          
          // if we are labeling the build, then we also uploaded the redirect files, so list those here too
          if (argv.build_label) {
            let label_ext = argv.build_label === '.' ? '' : '.' + argv.build_label;
            top_files.forEach((file_name) => {
              dependent_files.push(ms.root_dir + config.sdk_component + '/' + file_name + label_ext);
            });
          }
          
          // we somewhat arbitrarily use the first "top" file
          // as the key for this database entry
          let name = ms.root_dir + config.sdk_component;
          if (top_files.length > 0)
            name = ms.root_dir + config.sdk_component + '/' + top_files[0] + '.' + loc_hashes[top_files[0]];

          let params = {
            TableName: config.ddb_table,
            Item: {
              Name: { S: name },
              DependentFiles: { SS: dependent_files },
              LastPostedDate: { S: Date() },
              LastPostedBy: { S: argv.git_user }
            }
          };
          
          if (argv.emcc_ver)
            params.Item.Emcc = { S: argv.emcc_ver };
          if (argv.nacl_ver)
            params.Item.NaCl = { S: argv.nacl_ver };
          if (argv.git_head)
            params.Item.GitCommit = { S: argv.git_head };
          if (argv.git_origin_url)
            params.Item.GitOriginURL = { S: argv.git_origin_url };
          if (argv.git_branch)
            params.Item.GitBranch = { S: argv.git_branch };
          if (argv.build_os)
            params.Item.BuildOS = { S: argv.build_os };
          if (argv.git_status !== '')
            params.Item.GitMods = { S: argv.git_status };
            
          //aws.config.credentials = new aws.TemporaryCredentials({RoleArn: 'arn:aws:iam::123123123123:role/DynamoDB-Admin'});
          let ddb = new aws.DynamoDB();
          verbose_log('adding database entry for: ' + name);
          ddb.putItem(params, (error, data) => {
            if (error)
              reject(error);
            else
              resolve();
          });
        });

        db_prom.then(() => {
          // when everything is done, display some useful information to the user
          console.log('');
          if (top_files.length > 0)
            console.log('posted label:     ' + ms.root_dir + config.sdk_component + '/' + top_files[0] + '.' + loc_hashes[top_files[0]]);
          else
            console.log('posted label:     ' + ms.root_dir + config.sdk_component);
          console.log('');
          console.log('entry point files:');
          top_files.forEach((file_name) => {
            let suffix = '';
            if (argv.build_label) {
              if (argv.build_label !== '.')
                suffix = '.' + argv.build_label;
            } else
              suffix = '.' + loc_hashes[file_name];
            console.log(config.http_loc + ms.root_dir + config.sdk_component + '/' + file_name + suffix);
          });
          console.log('');
        },
        (err) => {
          ms.assert(err)
        });

      },
      (err) => {
        ms.assert(err);
      });
      
    },
    (err) => {
      ms.assert(err);
    });

  } catch(err) {
    ms.assert(err);
  }
  
});
