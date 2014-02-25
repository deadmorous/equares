var fs = require('fs')
var mongoose = require('mongoose')
var ObjectId = mongoose.Schema.Types.ObjectId

var SimSchema = mongoose.Schema({
    name:           {type: String, index: {unique: true}},
    description:    String,
    definition:     String,
    user:           {type: ObjectId, index: true}
})

var Sim = mongoose.model('Sim', SimSchema, 'simulations')

// Refresh example simulations in database
function refreshExamples() {
    var dir = 'public/examples/'
    var nFilesLoaded = 0
    var nDirs
    
    function readFile(fileName) {
        var text = ""
        fs.createReadStream(dir + fileName)
            .on('data', function(data) { text += data })
            .on('end', function() {
                var obj = JSON.parse(text)
                var name = obj.name, description = obj.description
                delete obj.name
                delete obj.description
                console.log("  " + fileName + ": " + name + " (" + description + ")")
                Sim.create({
                    name: name,
                    description: description,
                    definition: JSON.stringify(obj),
                    user: null
                }, function(err, sim) {
                    if (err)
                        console.log(err)
                    if (++nFilesLoaded == nDirs)
                        console.log('... Finished loading example simulations')
                })
            })
    }

    fs.readdir(dir, function(err, files) {
        if (err)
            console.log(err)
        else Sim.remove({user: null}, function() {
            console.log('Loading example simulations to the database ...')
            nDirs = files.length
            for (var i=0; i<nDirs; ++i) {
                var fileName = files[i]
                readFile(fileName)
            }
        })
    })
}

refreshExamples()

module.exports = {
    Sim: Sim
}
