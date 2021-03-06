/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

// equares.js
var child_process = require('child_process');
var lins = require('line-input-stream');
var stream = require('stream');
var fs = require('fs')
var path = require('path')
var simulation = require('../simulation')
var auth = require('../auth')

var equares = {};

var equaresCachePath = path.normalize(__dirname + '/../equares-cache')
fs.mkdir(equaresCachePath, function() {
    fs.mkdir(path.join(equaresCachePath, 'equares'), function() {})
})

equares.programPath = (function() {
    var path = process.env['EQUARES_BIN'];
    if( path == undefined ) {
        err = 'EQUARES_BIN environment variable is missing';
        console.log( err );
        throw err;
        }
    return path + '/equares_con';
    })();

function StreamLineHistory(str, ls) {
    this.stream = str;  // Input/output stream
    this.lstream = ls;  // Line input stream
    this.history = [];
    }


function User(name, auth) {
    this.name = name
    this.auth = auth
    this.proc = undefined
    this.lastev = 0
    this.stdio = []     // Array for server process streams
}

equares.users = {};
equares.user = function(req) {
    var auth = req.isAuthenticated()   &&   req.user.activation_code === 'X'
    var name = auth? req.user.username: ''  // TODO: session id
    return equares.users[name] || (equares.users[name] = new User(name, auth))
}
equares.anonymousUser = function() {
    return equares.users[''] || (equares.users[''] = new User('', false))
}

User.prototype.runConfig = function(args) {
    function merge(a, b) {
        if (!(b instanceof Array))
            return a
        for (var n=b.length, i=0; i<n; ++i)
            a.push(b[i])
        return a
    }

    if (this.auth)
        return {
            cwd: 'users/' + this.name,
            args: merge(args, ['-c', equaresCachePath])
        }
    else
        return {
            cwd: equaresCachePath,
            args: merge(args, ['-b'])
        }
}

User.prototype.isRunning = function() {
    return this.proc !== undefined;
}

User.prototype.addStreamLine = function(code, line) {
    var h = this.stdio[code].history;
    h[h.length] = line;
}

User.prototype.initStream = function(code, str, optConsumer) {
    //console.log('initStream: consumer=', optConsumer);
    var user = this;
    var ls = lins(str);
    user.stdio[code] = new StreamLineHistory(str, ls);
    ls.setEncoding('utf8');
    ls.setDelimiter('\n');
    ls.on('error', function(err) {
        console.log(err + ', user ' + user.name);
    });
    ls.on('line', function(line) {
        var ch = code == 0 ?   '=>' :   code == 1 ?   '<=' :   '):';
        console.log('%s@equares %s %s', user.name, ch, line);
        user.addStreamLine(code, line);
        if(optConsumer !== undefined)
            optConsumer.write(line+'\n');
    });
}

User.prototype.stopped = function() {
    this.proc = undefined;
    this.lastev = 0;
    this.stdio = [];
}

User.prototype.start = function() {
    var user = this;
    console.log( 'Starting equares for user %s', user.name );
    var rc = this.runConfig(['-s'])
    this.proc = child_process.spawn(equares.programPath, rc.args, {cwd: rc.cwd});
    this.lastev = 1;
    this.proc.on('close', function() {
        console.log( 'equares for user %s has been closed', user.name );
        user.stopped();
    });
    this.proc.on('error', function() {
        console.log( 'equares for user %s terminated unexpectedly', user.name );
        user.stopped();
    });
    // console.log('start: user.proc.stdin = ', user.proc.stdin);
    user.initStream(0, new stream.PassThrough(), user.proc.stdin);
    user.initStream(1, user.proc.stdout);
    user.initStream(2, user.proc.stderr);
}

User.prototype.execCommand = function(command) {
    if( this.proc )
        this.stdio[0].stream.write(command + '\n');
}

User.prototype.stop = function() {
    if( this.proc ) {
        console.log( 'Stopping equares for user %s', this.name );
        this.proc.kill();

        // Do this here (in addition to 'stopped' handler)
        // in order isRunning to work correctly before stopped() get called.
        this.proc = undefined;
    }
}

User.prototype.toggle = function() {
    if (this.isRunning())
        this.stop();
    else
        this.start();
};

var commands = {}

function ensureAuth(req, res) {
    if (!req.isAuthenticated()) {
        res.send(401, 'You are not logged in')
        return false
    }
    if (req.user.activation_code !== 'X') {
        res.send(401, 'Your account is not activated yet.<br/> Please activate it to access Equares functionality')
        return false
    }
    return true
}


commands['toggle'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    var user = equares.user(req)
    user.toggle()
    res.end()
};

commands['stat'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    var user = equares.user(req)
    res.send(user.isRunning()? '1': '0')
};

commands['statEvent'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    res.writeHead(200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        'Connection': 'keep-alive'
    });
    res.write('\n');
    var user = equares.user(req)
    var goon = true;
    res.on('error', function() {
        // console.log('statEvent: stopping due to error');
        goon = false;
        });
    res.on('close', function() {
        // console.log('statEvent: stopping due to close');
        goon = false;
        });
    var lastev;
    (function checkStatEvents() {
        if( !goon )
            return;
        if( lastev !== user.lastev ) {
            lastev = user.lastev;
            res.write('data: ' + lastev + '\n\n');
            // console.log('statEvent: %s', lastev);
            }
        setTimeout( checkStatEvents, 200 );
    })();
};

commands['outputEvent'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    res.writeHead(200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        'Connection': 'keep-alive'
    });
    res.write('\n');
    var user = equares.user(req)
    var goon = true;
    var nstreams = 3;// user.stdio.length;
    var lines = [];
    for( var i=0; i<nstreams; ++i )
        lines[i] = 0;
    // console.log('=== outputEvent ===');
    res.on('error', function() {
        // console.log('outputEvent: stopping due to error');
        goon = false;
        });
    res.on('close', function() {
        // console.log('outputEvent: stopping due to close');
        goon = false;
        });
    (function checkOutputEvents() {
        if( !goon )
            return;
        var haveEvent = false;
        for( var i=0; i<nstreams; ++i ) {
            if( user.stdio[i] == undefined ) {
                lines[i] = 0;
                continue;
            }
            var a = user.stdio[i].history;
            if( lines[i] < a.length ) {
                res.write('data: ' + JSON.stringify({stream: i, text: a[lines[i]]}) + '\n\n');
                // console.log('outputEvent[%d]: %s', i, a[lines[i]]);
                ++lines[i];
                haveEvent = true;
            }
        }
        setTimeout( checkOutputEvents, haveEvent? 0: 200 );
    })();
};

commands['runSimulation'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    var user = equares.user(req)
    function startSim() {
        // Start server
        user.start();

        // Feed input
        var script = req.body.script,
            simulation = req.body.simulation
        var command = '===={\n' + script + '\nrunSimulation(\n' + JSON.stringify(simulation) + '\n)\n' + '====}'

        // deBUG, TODO: Remove
        fs.writeFile(user.runConfig([]).cwd + '/sim.json', JSON.stringify(simulation), {encoding: 'utf8'}, function() {})

        user.execCommand(command);
        res.send('Started simulation');
    }

    if (user.isRunning()) {
        // Run simulation after stopping the currently running server instance
        user.proc.on('close', startSim);
        user.stop();
    }
    else
        startSim();
}

commands['sync'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    var user = equares.user(req)
    var command = req.query.cmd
    if (!command.match(/^==\d+==\<$/))
        res.send(403)
    else {
        user.execCommand(command)
        res.send(user.isRunning(user)? '1': '0')
    }
};

commands['input'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    var user = equares.user(req)
    var command = req.query.cmd
    if (!command.match(/^==\d+==\:.*$/))
        res.send(403)
    else {
        user.execCommand(command)
        res.send(user.isRunning(user)? '1': '0')
    }
};

var equaresInfoCache = {};

function boxInfo(req, box, cb) {
    if (equaresInfoCache[box])
        cb(box, equaresInfoCache[box])
    else {
        var rc = equares.user(req).runConfig([equares.programPath, '-d', box])
        child_process.exec(rc.args.join(' '), {cwd: rc.cwd}, function (error, stdout, stderr) {
            var result = {
                stdout: stdout,
                stderr: stderr
            }
            if (error)
                result.error = error
            cb(box, equaresInfoCache[box] = JSON.stringify(result))
        })
    }
}

commands['requestInfo'] = function(req, res) {
    boxInfo(req, req.query.cmd, function(box, info) {
        res.send(info)
    })
}

commands['requestInfoEx'] = function(req, res) {
    var query = req.body
    var describeOptions = query.options || ''
    var rc = equares.user(req).runConfig(['-i', '-d'+describeOptions, 'box'])
    var stdin = 'box = new ' + query.type + '\n'
    for (var prop in query.props)
        stdin += 'box.' + prop + ' = ' + JSON.stringify(query.props[prop]) + '\n'
    var proc = child_process.spawn(equares.programPath, rc.args, {cwd: rc.cwd});
    var stdout = '', stderr = '', replied = false
    function reply(text) {
        if (!replied) {
            replied = true
            res.write(text)
            res.end()
        }
    }

    proc.stdout.on('data', function(chunk) {
        stdout += chunk.toString()
    })
    proc.stderr.on('data', function(chunk) {
        stderr += chunk.toString()
    })
    proc.stdin.on('error', function(){
        reply(JSON.stringify({error: -1, message: 'Failed to start equares'}))
    })
    proc.stdin.end(stdin)
    proc.on('close', function(code) {
        if (code === 0   &&   stderr.length === 0)
            reply(JSON.stringify(stdout))
        else
            reply(JSON.stringify({error: code, stdout: stdout, stderr: stderr}))
    });
    proc.on('error', function() {
        if (stderr.length > 0   ||   stdout.length > 0)
            // Process has started and returned a nonzero error code.
            // This means that 'close' will follow - we will reply there.
            // Note: Could not deduce reason for error from args, they are empty :(
            return;
        reply(JSON.stringify({error: -1, message: 'Failed to start equares'}))
    });
}

commands['quicksave'] = function(req, res) {
    req.session.simulation = req.body.simulation
    simulation.RecentSim.set(req)
    res.end()
}

commands['quickload'] = function(req, res) {
    function sessionRecentSim() {
        return req.session.simulation   ||
            JSON.stringify({
                name:           '',
                description:    '',
                info:           '',
                keywords:       [],
                script:         '',
                public:         false,
                definition:     JSON.stringify({boxes: [], links: []})
            })
    }

    if (req.isAuthenticated()   &&   req.user.activation_code === 'X') {
        simulation.RecentSim.get(req, function(sim) {
            if (typeof sim !== 'string')
                sim = sessionRecentSim()
            res.send(sim)
        })
    }
    else
        res.send(sessionRecentSim())
}

// Finds build dirs for the specified simulation from database
function buildDirs(sim, cb) {
    var rc = equares.anonymousUser().runConfig(['-i', '-q'])
    var stdin = ''
    if (sim.script)
        stdin += sim.script + '\n'
    stdin += 'print(buildDirs(' + sim.definition + ').join("\n"))\n'
    var proc = child_process.spawn(equares.programPath, rc.args, {cwd: rc.cwd});
    var stdout = '', stderr = '', replied = false
    function reply(text) {
        if (!replied) {
            replied = true
            sim.buildDirs = text.split('\n')
            cb(sim)
        }
    }

    proc.stdout.on('data', function(chunk) {
        stdout += chunk.toString()
    })
    proc.stderr.on('data', function(chunk) {
        stderr += chunk.toString()
    })
    proc.stdin.on('error', function(){
        console.log('buildDirs: Failed to start equares')
        reply('')
    })
    proc.stdin.end(stdin)
    proc.on('close', function(code) {
        if (code === 0   &&   stderr.length === 0)
            reply(stdout)
        else {
            console.log('buildDirs: Failed to retrieve build dirs: exit code=' + code + ', stderr:\n' + stderr + '\n\nstdout: ' + stdout)
            reply('')
        }
    });
    proc.on('error', function() {
        if (stderr.length > 0   ||   stdout.length > 0)
            // Process has started and returned a nonzero error code.
            // This means that 'close' will follow - we will reply there.
            // Note: Could not deduce reason for error from args, they are empty :(
            return;
        console.log('Failed to start equares')
        reply('')
    });
}

commands['savesim'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    var sim = JSON.parse(req.body.simulation)
    if (typeof sim.name !== 'string'   ||   !sim.name.match(/\S/))
        return res.send(400, 'Please enter a non-empty simulation name')
    sim.date = new Date()
    sim.user = req.user.id
    if (typeof sim.public != 'boolean')
        sim.public = false

    function save() {
        buildDirs(sim, function(sim) {
            simulation.Sim.upsert(sim, function(err, sim) {
                if (err) {
                    console.log(err)
                    res.send(500, 'Failed to save simulation')
                }
                else
                    res.end()
            })
        })
    }

    var overwrite = JSON.parse(req.body.overwrite)
    if (overwrite)
        save()
    else
        simulation.Sim.have(sim, function(err, count) {
            if (err)
                res.send(500)
            else if (count > 0)
                res.send(403)
            else
                save()
        })
}

commands['editsim'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    simulation.Sim.findBySpec(req.query.sim, function(err, s) {
        if (err) {
            console.log(err)
            res.send(500, err)
        }
        else if (s) {
            var sim = s.toObject()
            if (!s.user || s.user.toString() != req.user._id.toString())
                return res.send(403, 'Permission denied')
            var upd = {}
            if (req.query.name)
                upd.name = req.query.name
            if (req.query.public)
                upd.public = req.query.public
            simulation.Sim.update({user: sim.user, name: sim.name}, {$set: upd}, function(err, sim) {
                if (err) {
                    if (err.code == 11001)
                        res.send(403, 'New name is already in use')
                    else {
                        console.log(err)
                        res.send(403, err)
                    }
                }
                else
                    res.send('Simulation has been modified')
            })
        }
        else
            res.send(404, 'No such simulation')
    })
}

commands['delsim'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    simulation.Sim.findBySpec(req.query.sim, function(err, s) {
        if (err) {
            console.log(err)
            res.send(500, err)
        }
        else if (s) {
            if (!s.user || s.user.toString() != req.user._id.toString())
                return res.send(403, 'Permission denied')
            s.remove(function(err) {
                if (err) {
                    console.log(err)
                    res.send(500, err)
                }
                else
                    res.end()
            })
        }
        else
            res.send(404, 'No such simulation')
    })
}

commands['simtextfile'] = function(req, res) {
    if (!ensureAuth(req, res))
        return
    var url = '/users/' + req.user.username + '/' + req.query.file
    var file = path.normalize(__dirname + '/..' + url)
    function makeTable(d) {
        d = d.replace('\r', '').split('\n')
        var n = d.length
        if (n < 1)
            return '<table></table>'
        var rows = ''
        for (var i=0; i<n; ++i) {
            var cells = ''
            var r = d[i].split('\t'), m = r.length
            for (var j=0; j<m; ++j)
                cells += '<td>' + r[j] + '</td>'
            rows += '<tr>' + cells + '</tr>'
        }
        return '<table>' + rows + '</table>'
    }
    fs.stat(file, function(err, stats) {
        if (err)
            return res.send(404)
        if (stats.size === 0)
            return res.send('empty file')
        var sizeLimit = 100*1024, sizeToRead = stats.size <= sizeLimit? stats.size: sizeLimit
        var replied = false
        function reply(d) {
            if (replied)
                return
            replied = true
            if (typeof d == 'number')
                return res.send(d)
            var html = makeTable(d)
            if (stats.size > sizeLimit)
                html += '<br/>(truncated)'
            html += '<br/><a href="' + url + '" target="_blank">download</a>'
            res.send(html)
        }

        var data = ''
        fs.createReadStream(file, {encoding: 'utf8', start: 0, end: sizeToRead-1})
            .on('data', function(d) {
                data += d
            })
            .on('end', function() {
                reply(data)
            })
            .on('error', function(err) {
                console.log(err)
                reply(500)
            })
    })
}

// Cleanup service
var removingOldBuildDirs = false
function removeOldBuildDirs() {
    if (removingOldBuildDirs)
        return
    removingOldBuildDirs = true
    console.log('Removing outdated user files ...')
    var usersRoot = path.normalize(__dirname + '/../users')

    var tasks = 0, finished = false
    function proceed() {
        --tasks
        if (!finished)
            return
        if (tasks < 0) {
            console.log('... Finished removing outdated user files')
            removingOldBuildDirs = false
        }
    }

    var now = (new Date).getTime()
    var maxage = 23 // Maximum age of user result file, in hours
    var rmrf = function(filePath) {
        child_process.exec('rm -rf ' + filePath, function (error, stdout, stderr) {
            if (error)
                console.log(stderr)
            proceed()
        })
    }

    auth.User.find({}, {username: 1}).stream()
        .on('data', function(doc) {
            var username = doc.username, userId = doc._id,
                userDir = path.join(usersRoot, username),
                userBuildDir = path.join(userDir, 'equares')
            function inspectBuildDir(name) {
                var buildPath = path.join(userBuildDir, name)
                    fs.stat(buildPath, function(err, stats) {
                        if (err)
                            return proceed()
                        if (!stats.isDirectory()) {
                            // Remove regular file
                            console.log('Removing unexpected regular file ' + buildPath)
                            return rmrf(buildPath)
                        }
                        simulation.Sim.count({user: userId, buildDirs: name}, function(err, n) {
                            if (err || n > 0)
                                return proceed()
                            console.log('Removing old build directory ' + buildPath)
                            rmrf(buildPath)
                        })
                    })
                }
            function inspectUserFile(name) {
                if (name == 'equares')
                    return proceed()    // Ignore build dir root
                var filePath = path.join(userDir, name)
                fs.stat(filePath, function(err, stats) {
                    if (err)
                        return proceed()
                    var age = (now - stats.mtime.getTime())/(1000*60*60)
                    if (age < maxage)
                        return proceed()
                    console.log('Removing file/dir ' + filePath + ' (age ' + age + ' h)')
                    rmrf(filePath)
                })
            }

            ++tasks
            fs.readdir(userBuildDir, function(err, files) {
                if (err)
                    return proceed()
                tasks += files.length
                proceed()
                for (var i=0; i<files.length; ++i)
                    inspectBuildDir(files[i])
            })

            ++tasks
            fs.readdir(userDir, function(err, files) {
                if (err)
                    return proceed()
                tasks += files.length
                proceed()
                for (var i=0; i<files.length; ++i)
                    inspectUserFile(files[i])
            })
        })
        .on('error', function (err) {
            console.log(err)
        })
        .on('close', function () {
            finished = true
            proceed()
        })
}

setTimeout(removeOldBuildDirs, 1000)
setInterval(removeOldBuildDirs, 1000*60*60*24)


module.exports = function() {
    return function(req, res, next) {
        var name = req.path.substr(1)
        var cmd = commands[name]
        if (cmd)
            cmd(req, res)
        else
            res.send(404, 'Command not found')
    }
}

module.exports.boxInfo = boxInfo
