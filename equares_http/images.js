/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

var fs = require('fs')
var mongoose = require('mongoose')
var textSearch = require('mongoose-text-search');
var auth = require('./auth')
var cp = require('child_process')
var dbsearch = require('./dbsearch')

var ObjectId = mongoose.Schema.Types.ObjectId

var ImgSchema = mongoose.Schema({
    name:           {type: String, index: true},
    title:          String,
    date:           Date,
    keywords:       {type: [String], index: true},
    contentType:    String,
    data:           String,
    preview:        String,
    user:           {type: ObjectId, index: true}
})

// give our schema text search capabilities
ImgSchema.plugin(textSearch);

ImgSchema.index({name: 1, user: 1}, {unique: true})
ImgSchema.index({name: 'text', title: 'text', keywords: 'text'})

ImgSchema.statics.upsert = function(img, done) {
    this.update({user: img.user, name: img.name}, img, {upsert: true}, done)
}
ImgSchema.statics.have = function(img, done) {
    this.count({user: img.user, name: img.name}, done)
}

var Img = mongoose.model('Img', ImgSchema, 'images')

function pickImage(req, res) {
    res.render('pick-image', {req: req})
}

function imageThumbnails (req, res) {
    dbsearch(req, {
        model: Img,
        project: {name: 1, user: 1, keywords: 1, title: 1},
        pageSize: 100
    }, function(result) {
        if (result.err)
            return res.send(result.err.code || 500, result.err.message || result.err.data)
        var count = result.records.length
        if (result.pagenum)
            res.write('<div class="img-pagenum">' + result.pagenum + '</div>')
        res.write('<div class="img-previews">')
        for (var i=0; i<count; ++i) {
            var img = result.records[i],
                a = ['', 'img', img.user? img.user: '-', 'preview', img.name],
                previewName  = a.join('/')
            a.splice(3, 1)
            var name = a.join('/')
            res.write('<div class="img-container">')
            res.write('<a href="' + name + '"><img alt="' + img.name + '" title="' + img.title + '" src="' + previewName + '" width="100" height="100"/></a>')
            if (img.edit)
                res.write(
                    '<div class="img-control">' +
                        '<div class="img-tool img-edit"></div><div class="img-tool img-remove"></div>' +
                        '<div class="img-kw">' + img.keywords.join(', ') + '</div>' +
                    '</div>'
                )
            res.write('</div>')
        }
        res.end('</div>')
    })
}

function getImage(req, res) {
    var a = req.path.split('/'), preview
    if(a.length == 3)
        preview = false
    else if (a.length == 4 && a[2] == 'preview')
        preview = true
    else
        return res.send(404, 'Image is not found')
    var user = a[1], name = a[preview? 3: 2]

    function serve() {
        Img.findOne({name: name, user: user}, function(err, img) {
            if (err) {
                console.log(err)
                res.send(500)
            }
            else if (img) {
                if (preview) {
                    res.contentType('image/png')
                    res.end(img.preview, 'binary')
                }
                else {
                    res.contentType(img.contentType)
                    res.end(img.data, 'binary')
                }
            }
            else
                res.send(404, 'Image is not found')
        })
    }

    if (user == '-') {
        user = null
        serve()
    }
    else
        auth.User.findUser(user, function(err, userId) {
            if (err) {
                console.log(err)
                return res.send(500, 'Unable to resolve user')
            }
            else if (!userId)
                return res.send(404, 'No such user')
            user = userId._id
            serve()
        })
}

function imgresize(img, param, cb)
{
    var dir = process.env["EQUARES_BIN"];
    param.push(64)
    var imgresize = cp.spawn(dir + '/imgresize', param, {cwd: dir})
    var resized = '', stderr = ''
    imgresize.stdin.on('error', function(err) {
        console.log('imgresize input error')
        console.log(err)
    })
    var sss = new Buffer(img.data, 'binary').toString('base64')
    imgresize.stdin.end(sss, 'binary')
    imgresize.stdout.setEncoding('binary')
    imgresize.stdout.on('data', function(data) {
        resized += new Buffer(data, 'base64').toString('binary')
    })
    imgresize.on('close', function(code) {
        // console.log('Image has been resized, code=' + code)
        if (code || stderr) {
            var s = 'Image resize error (code ' + code + ')'
            if (stderr)
                s += ': ' + stderr
            console.log(s)
            cb(s)
        }
        else
            cb(null, resized)

    })
    imgresize.on('error', function(err) {
        console.log('Unable to run imgresize')
        console.log(err)
    })
    imgresize.stderr.on('data', function(data) {
        stderr += data
    })
}

function uploadImage(req, res) {
    if (!req.isAuthenticated())
        return res.send(401, 'You are not logged in')
    var img = JSON.parse(req.body.img)
    img.date = new Date()
    img.user = req.user.id

    function save() {
        delete img.overwrite
        imgresize(img, [100, 100], function(err, resized) {
            if (err)
                res.send(500, 'Failed to generate image preview')
            else {
                img.preview = resized
                Img.upsert(img, function(err, doc) {
                    if (err) {
                        console.log(err)
                        res.send(500, 'Failed to upload image')
                    }
                    else
                        res.send(['', 'img', req.user.username, img.name].join('/'))
                })
            }
        })
    }

    if (img.overwrite)
        save()
    else
        Img.have(img, function(err, count) {
            if (err)
                res.send(500)
            else if (count > 0)
                res.send(403, 'Image with the specified name already exists')
            else
                save()
        })
}

function copyProps(dst, src) {
    for (var i in src)
        dst[i] = src[i]
    return dst
}

function editImage(req, res) {
    if (!req.isAuthenticated())
        return res.send(401, 'You are not logged in')
    var img = JSON.parse(req.body.img)
    if (typeof img.name != 'string')
        return res.send(400, 'Invalid query')
    Img.findOne({name: img.name, user: req.user.id}, function(err, s) {
        if (err) {
            console.log(err)
            res.send(500, err)
        }
        if (s) {
            function save() {
                delete img.name
                copyProps(s, img)
                s.save(function(err) {
                    if (err) {
                        console.log(err)
                        res.send(500, 'Failed to modify image')
                    }
                    else
                        res.send(['', 'img', req.user.username, img.name].join('/'))
                })
            }
            if (img.data) {
                imgresize(img, [100, 100], function(err, resized) {
                    if (err)
                        res.send(500, 'Failed to generate image preview')
                    else {
                        img.preview = resized
                        save()
                    }
                })
            }
            else
                save()
        }
        else
            res.send(404, 'No such image')
    })
}

function removeImage(req, res) {
    if (!req.isAuthenticated())
        return res.send(401, 'You are not logged in')
    var name = req.query.name
    if (typeof name != 'string')
        return res.send(400, 'Invalid query')
    Img.findOne({name: name, user: req.user.id}, function(err, s) {
        if (err) {
            console.log(err)
            res.send(500, err)
        }
        if (s)
            s.remove(function(err) {
                if (err) {
                    console.log(err)
                    res.send(500, err)
                }
                else
                    res.end()
            })
        else
            res.send(404, 'No such image')
    })
}

function refreshImages() {
    var dir = 'public/meta/'
    var imginfo = fs.readFileSync(dir + 'images.json', {encoding: 'utf8'})
    var date = new Date(2014, 3, 29)
    imginfo = JSON.parse(imginfo)
    for (var i=0; i<imginfo.length; ++i) {
        (function(img) {
            img.date = date
            img.user = null
            img.data = fs.readFileSync(dir + img.name).toString('binary')
            imgresize(img, [100, 100], function(err, resized) {
                if (err)
                    console.log('ERROR: Failed to generate image preview')
                else {
                    img.preview = resized
                    Img.upsert(img, function(err, doc) {
                        if (err)
                            console.log(err)
                    })
                }
            })
        })(imginfo[i])
    }
}

refreshImages()

module.exports = function(app) {
    app.get('/pick-image', pickImage)
    app.get('/image-thumbnails', imageThumbnails)
    app.use('/img', getImage)
    app.post('/upload-image', uploadImage)
    app.post('/edit-image', editImage)
    app.get('/remove-image', removeImage)
}
