var equaresui = {};

(function() {

function wrap(tag) { return $("<" + tag + "></" + tag + ">") }

equaresui.setSceneSource = function() {
    this.clear();
    var layoutOptions = {
        type: "horizontal",
        fixed: true,
        toolhandle: false
    };
    this.setTitle( "Simulation editor" );
    var layout = this.setLayout( layoutOptions );
    
    var boxCell = layout.add( { title: "Boxes", width: {min: 50, max: 300} } );
    var docCell = layout.add( { title: "Quick reference", width: {min: 50, max: 500} } );
    $(docCell.dom).append(wrap("h1").html("Quick reference")).append("TODO")
    docCell.header().minimize(docCell);
    var boxDiv = $(boxCell.dom);
    var boxHelp = $("body").append('<div id="scheme-boxhelp"></div>').children("#scheme-boxhelp").hide();
    boxDiv.addClass('scheme-boxlist');
    var draggingBox = false;

    var boxItemHelpText = function(item, caption) {
        if (item === undefined)
            return "";
        var text = "<h2>" + caption + "</h2>";
        if (item.length == 0)
            text += "None."
        else {
            text += "<ul>";
            for (var index in item) {
                var p = item[index];
                text += "<li>"
                text += "<b>" + p.name + "</b>"
                if (p.help)
                    text += "<br/>" + p.help;
                if (p.format) {
                    var f = new equaresBox.PortFormat(p)
                    text += "<br/>Format: " + f.toHtml()
                }
                text += "</li>"
            }
            text += "</ul>"
        }
        return text;
    }

    var boxHelpText = function(box, info)
    {
        var text = "<h1>" + box + "</h1>";
        text += (info.help? info.help: "No help available") + "<br>";
        text += boxItemHelpText(info.inputs, "Input ports");
        text += boxItemHelpText(info.outputs, "Output ports");
        text += boxItemHelpText(info.properties, "Fixed parameters");
        return text;
    }

    var HoverPopup = equaresui.HoverPopup = function(popup, enter, timeout) {
        timeout = timeout || 500
        var nin = 0
        var ncheck = 0
        function check() {
            if (ncheck == 0) {
                setTimeout(hide, timeout)
                ++ncheck
            }
        }
        function hide() {
            --ncheck
            if (nin == 0)
                 popup.hide('fast')
             else
                 check()
        }
        this.enter = function() {
            ++nin;
            enter.call(this, popup)
        }
        this.leave = function() {
            --nin;
            check();
        }
        this.target = function(element) {
            $(element).hover(this.enter, this.leave)
        }
    }

    // Fill box div with box types
    ;(function() {
        var boxes = equaresBox.boxTypes
        var text = '<h1>Boxes</h1><table>';
        for (var i in boxes)
            text += '<tr></td><td><img src="images/question.png"></td><td class="scheme-boxlist-box">' + boxes[i] + '</tr>';
        text += '</table>';
        boxDiv.append(text);
        boxDiv.find('tr:odd').addClass('odd');
        boxDiv.find('tr:even').addClass('even');
        var hpBoxHelp = new HoverPopup(boxHelp, function() {
            if (draggingBox)
                return;
            var o = $(this);
            var pos = o.offset();
            pos.left += o.width();
            var box = o.parent().next().text();
            var info = equaresBox.boxInfo[box]
            boxHelp
                .html(boxHelpText(box, info));
            var wh = $(window).height(),   bh = boxHelp.outerHeight();
            if (pos.top + bh > wh)
                pos.top = Math.max(0,wh-bh);
            boxHelp
                .show('fast')
                .offset(pos);
        })
        boxDiv.find('td').find('img').each(function() { hpBoxHelp.target(this) })

        boxDiv.find(".scheme-boxlist-box")
            .click(function(){
                var box = $(this).text();
                schemeEditor.newBox(box).select()
            })
            .draggable({
                helper: function() {
                    return $("body")
                        .append('<div id="scheme-boxdrag">' + $(this).text() + '</div>')
                        .children("#scheme-boxdrag");
                },
                start: function() { draggingBox = true; },
                stop: function() { draggingBox = false; },
                scope: "newBox"
            });
    })()

    var schemeCell = layout.add( { title: "Scheme" } );
    var schemeEditor = ctmEquaresSchemeEditor.newEditor(schemeCell.dom)

    var schemeDiv = $(schemeCell.dom);
    schemeDiv.droppable({
        scope: "newBox",
        drop: function(event, ui) {
            var box = ui.helper.text();
            schemeEditor.newBox(box, ui).select()
        }
    });

    var settingsCell = layout.add( { title: "Settings" } );
    var settingsLayout = settingsCell.setLayout({type: "vertical", fixed: true});
    var propsCell = settingsLayout.add( { title: "Properties" } );
    var extrasCell = settingsLayout.add( { title: "Extras" } );
    ;(function() {
        var resizing = 0
        extrasCell.addResizeHandler(function() {
            if (++resizing == 1)
                $(this.dom).find("textarea.linenum-text").resize()
            --resizing
        })
    })()
    var extrasDiv = $('<div id="scheme-box-extras"></div>').appendTo($(extrasCell.dom))

    var portHelp = $('<div id="scheme-porthelp"></div>').appendTo($("body")).hide();
    var hpPortHelp = equaresui.hpPortHelp = new HoverPopup(portHelp, function() {
        var o = $(this)
        var pos = o.offset()
        pos.left += this.getBBox().width + 5
        var d = this.__data__
        var f = d.getFormat()
        portHelp.html(d.info.name + ' (' + d.type + ')<br/>Format: ' + f.toHtml())
        portHelp
            .show('fast')
            .offset(pos)
    },
    300)

    var statusHelp = $('<div id="scheme-statushelp"></div>').appendTo($("body")).hide();
    var hpStatusHelp = equaresui.hpStatusHelp = new HoverPopup(statusHelp, function() {
        var o = $(this);
        var pos = o.offset();
        pos.left += this.getBBox().width + 5;
        var d = this.parentElement.__data__;
        var msg = d.status.text
            .replace(/ /g, "&nbsp;")
            .replace(/\</g, "&lt;")
            .replace(/\>/g, "&gt;")
            .replace(/\n/g, "<br>")
            .replace(/\r/g, "")
        statusHelp.html(msg);
        if (d.status.level == "error")
            statusHelp.addClass("error")
        else
            statusHelp.removeClass("error")
        statusHelp
            .show('fast')
            .offset(pos);
    },
    300)

    var propsDiv = $('<div id="scheme-box-props"></div>').appendTo($(propsCell.dom))
    function validateBoxProp(value, userType, setter) {
        var s = value.toString().trim()
        var ok = true
        switch(userType) {
        case 'i':
            ok = /^[+-]?[0-9]+$/.test(s)
            value = +s
            break
        case 'd':
            // Regexp is copied from here: http://regexlib.com/REDetails.aspx?regexp_id=185
            ok = /^[+-]?([0-9]*\.?[0-9]+|[0-9]+\.?[0-9]*)([eE][+-]?[0-9]+)?$/.test(s)
            value = +s
            break
        }
        setter(value, ok)
    }

    function makeEditor(host, prop) {
        var t = prop.userType
        if (t === undefined)
            host.html("N/A")
        else if (t instanceof Array)
            host.html("TODO: Array")
        else if (t instanceof Object) {
            var table = wrap("table").appendTo(host), odd = false
            for (var name in t) {
                var row = wrap("tr").appendTo(table)
                row.addClass(odd ? "odd": "even")
                row.append(wrap("td").html(name))
                var tdVal = wrap("td").appendTo(row)
                ;(function(){    // Important: function provides closure for name_
                    var name_ = name
                    makeEditor(tdVal,
                        {
                            name: name,
                            userType: t[name],
                            getter: function() { return prop.getter()[name_] },
                            setter: function(val) {
                                var v = prop.getter()
                                v[name_] = val
                                prop.setter(v)
                            }
                        })
                })()
                odd = !odd
            }
        }
        else if (typeof(t) == "string") {
            switch (t) {
            case 's': case 'i': case 'd':
                $('<input type="text">')
                    .attr("value", prop.getter().toString())
                    .change(function() {
                        var e = this
                        validateBoxProp(this.value, t, function(value, ok) {
                            if (ok)
                                prop.setter(value)
                            e.value = prop.getter()
                            $(e).removeClass('modified')
                        })
                    })
                    .keypress(function() {
                        $(this).addClass('modified')
                    })
                    .appendTo(host)
                break
            case 'i:*':
                $('<input type="text">')
                    .attr("value", prop.getter().toString())
                    .change(function() {
                        var val = this.value.match(/-?[0-9]+/g) || []
                        for (var i=0; i<val.length; ++i)
                            val[i] = +val[i]
                        prop.setter(val)
                        this.value = prop.getter().toString()
                        $(this).removeClass('modified')
                    })
                    .keypress(function() {
                        $(this).addClass('modified')
                    })
                    .appendTo(host)
                break
            case 'b':
                $('<input type="checkbox">')
                    .attr("checked", prop.getter())
                    .change(function() {
                        prop.setter(this.checked)
                        this.checked = prop.getter()
                    })
                    .appendTo(host)
                break
            case 't':
                $('<input type="button">')
                    .attr("value", "...")
                    .click(function() {
                        extrasDiv.html("")
                        var textarea
                        wrap("div").attr("id", "scheme-box-extras-hdr")
                            .append(wrap("h1").html(prop.name))
                            .append(
                                 $('<input type="button" class="scheme-box-extras-ok" value="Ok">')
                                    .click(function() {
                                        prop.setter(textarea[0].value)
                                        textarea.removeClass('modified')
                                    })
                            )
                            .appendTo(extrasDiv)
                        wrap("div").attr("id", "scheme-box-extras-text")
                            .append(textarea = wrap("textarea"))
                            .appendTo(extrasDiv)
                        textarea.linenum()
                        textarea[0].value = prop.getter()
                        textarea.keypress(function() {
                            $(this).addClass('modified')
                        })
                        textarea.focus()
                    })
                    .appendTo(host)
                break
            case 'BoxType':
                var editor = wrap("select").appendTo(host)
                function findSelIndex() {
                    var index = -1
                    var v = prop.getter()
                    var boxes = equaresBox.boxTypes
                    for(var i=0; i<boxes.length; ++i) {
                        var b = boxes[i]
                        editor.append(wrap("option").attr("value", b).html(b))
                        if (b == v)
                            index = i
                    }
                    return index
                }
                var edom = editor[0]
                edom.selectedIndex = findSelIndex()
                editor.change(function() {
                    var type = this[edom.selectedIndex].value
                    prop.setter(type)
                    var i = findSelIndex()
                    if (edom.selectedIndex != i)
                        edom.selectedIndex = i
                })
                break
            default:
                if (t[0] == 'f') {
                    // Edit a combination of flags
                    var tx = {}
                    function hasFlag(flag) {
                        var v = prop.getter()
                        for(var i=0; i<v.length; ++i)
                            if (v[i] == flag)
                                return true
                        return false
                    }
                    var flags = t.substr(1).match(/\w+/g)
                    var flagProp = { name: prop.name, userType: {} }
                    for (var i=0; i<flags.length; ++i)
                        flagProp.userType[flags[i]] = 'b'
                    flagProp.getter = function() {
                        var result = {}
                        for (var i=0; i<flags.length; ++i) {
                            var flag = flags[i]
                            result[flag] = hasFlag(flag)
                            }
                        return result
                        }
                    flagProp.setter = function(val) {
                        var result = []
                        for (var i=0; i<flags.length; ++i) {
                            var flag = flags[i]
                            if (val[flag])
                                result.push(flag)
                        }
                        prop.setter(result)
                    }
                makeEditor(host, flagProp)
                }
                else
                    host.html("TODO: " + t)
                break
            }
        }
    }

    function loadProps(title, props, options) {
        options = options || {}
        var makeGetter = options.makeGetter || function(pname, props) { return function() { return props[pname] } },
            makeSetter = options.makeSetter || function(pname, props) { return function(value) {
                props[pname] = value
                schemeEditor.modify()
            } }
        extrasDiv.html("")
        propsDiv.html("")
        var hdr = wrap("h1").html(title).appendTo(propsDiv)
        var table = wrap("table").appendTo(propsDiv)
        var odd = true
        for (pname in props) {
            odd = !odd
            var p = props[pname]
            var row = wrap("tr").appendTo(table)
            row.addClass(odd? "odd": "even")
            row.append(wrap("td").html(pname))
            var userType = p.userType
            if (!userType) switch(typeof p) {
                case "number":   userType = 'd';   break
                default:   userType = 's';   break
            }
            makeEditor(
                wrap("td").appendTo(row), {
                    name: pname,
                    userType: userType,
                    getter: p.getter instanceof Function ?   p.getter :   makeGetter(pname, props),
                    setter: p.setter instanceof Function ?   p.setter :   makeSetter(pname, props)
                }
            )
        }
        table.find("td:first").next().children().first().focus()
    }

    equaresui.selectBox = function(box) {
        if (box) {
            var props = {
                name: {
                    userType: 's',
                    getter: function() { return box.name },
                    setter: function(newName) {
                        box.rename(newName)
                        propsDiv.children('h1').first().html(box.name)
                    }
                },
                type: {
                    userType: 'BoxType',
                    getter: function() { return box.type },
                    setter: function(newType) {
                        box.changeType(newType)
                    }
                }
            }
            for (var pname in box.props)
                props[pname] = { userType: box.propType(pname) }
            loadProps(box.name, props, {
                makeGetter: function(pname, props) { return function() { return box.prop(pname) } },
                makeSetter: function(pname, props) { return function(value) {
                    box.prop(pname, value)
                } }
            })
        }
        else
            loadProps("Simulation properties", simProps)
    }
    
    // Load simulation properties
    var simProps = {
        name: "",
        description: ""
    };
    equaresui.selectBox(null)

    function beforeLoadScheme()
    {
        var loadingProgress = $("#loading-progress")
        loadingProgress.progressbar("value", 0)
        $("#loading-progress-overlay").show()
    }
    function loadScheme(obj, modified)
    {
        var loadingProgress = $("#loading-progress")
        schemeEditor.import(obj,
            function() {
                loadingProgress.progressbar("value", 100)
                simProps.name = obj.name || ""
                simProps.description = obj.description || ""
                equaresui.selectBox(null)
                schemeEditor.modified = modified
            },
            function(percent) {
                loadingProgress.progressbar("value", percent)
            }
        )
    }

    equaresui.loadExample = function(exampleName) {
        beforeLoadScheme()
        $.get(exampleName)
            .done(function(obj) {
                loadScheme(obj, true)
            })
            .fail(function() {
                alert('Failed to load example')
                $("#loading-progress-overlay").hide()
            })
    }

    equaresui.openScheme = function(fileToLoad) {
        // var fileToLoad = document.getElementById("fileToLoad").files[0];
        var fileReader = new FileReader();
        fileReader.onload = function(fileLoadedEvent)
        {
            var textFromFileLoaded = fileLoadedEvent.target.result;
            var obj = JSON.parse(textFromFileLoaded)
            loadScheme(obj, true)
        };
        beforeLoadScheme()
        fileReader.readAsText(fileToLoad, "UTF-8");
    }

    equaresui.saveScheme = function(fileName) {
        if (arguments.length < 1)
            fileName = "equares-scheme.eqs"
        var obj = schemeEditor.export()
        $.extend(obj, simProps)
        var schemeText = JSON.stringify(obj)
        var b = new Blob([schemeText], {type: "text/plain"})

        var downloadLink = document.createElement("a");
        downloadLink.download = fileName;
        downloadLink.innerHTML = "Download File";
        if (window.webkitURL != null) {
            // Chrome allows the link to be clicked
            // without actually adding it to the DOM.
            downloadLink.href = window.webkitURL.createObjectURL(b);
        }
        else {
            // Firefox requires the link to be added to the DOM
            // before it can be clicked.
            downloadLink.href = window.URL.createObjectURL(b);
            downloadLink.onclick = function(event) { document.body.removeChild(event.target); }
            downloadLink.style.display = "none";
            document.body.appendChild(downloadLink);
        }
        downloadLink.click();

    }
    equaresui.runScheme = function() {
        var simulation = schemeEditor.exportSimulation()
        function dummyStopSim() {}
        equaresui.stopSimulation = dummyStopSim
        $.ajax("cmd/runSimulation", {data: simulation, type: "POST", cache: false})
            .done(function() {
                var dlg = $("#running-simulation")
                var stopButton = $(".ui-dialog-buttonpane button:contains('Stop')")
                stopButton.button("enable")
                dlg.dialog("open")
                var status = dlg.find(".status").html("running")
                var running = true
                equaresui.stopSimulation = function() {
                    $.ajax("cmd/toggle", {type: "GET", cache: false})
                }

                var outfiles = $("#simulation-output-files").html("").css("text-align", "center")

                var equaresStatEvent = new EventSource("cmd/statEvent");
                equaresStatEvent.onmessage = function(event) {
                    if (+event.data === 0) {
                        // Simulation has finished
                        status.html("finished");
                        running = false
                        stopButton.button("disable")
                        equaresui.stopSimulation = dummyStopSim
                        equaresStatEvent.close()
                        equaresOutputEvent.close()
                    }
                }
                var equaresOutputEvent = new EventSource("cmd/outputEvent");
                var rxFile = /^==([0-9])+==\> file: (.*)/,
                    rxSync = /^==([0-9])+==\> sync/;
                var outFileInfo = {}
                var started = false
                var prefix, syncToken
                var faStarted = false   // File announcement started
                var faFinished = false  // File announcement finished
                equaresOutputEvent.onmessage=function(event) {
                    data = JSON.parse(event.data);
                    var str = data.text, i, fi, m
                    switch (data.stream) {
                    case 0: return  // Ignore stdin
                    case 1:         // Parse stdout
                        if (!started) {
                            // Wait for start token
                            m = str.match(/^==([0-9])+==\> started/)
                            if (m && m.length > 1) {
                                prefix = "==" + m[1] + "==> "
                                syncToken = "==" + m[1] + "==<"
                                started = true
                            }
                            return
                        }
                        if (!str.match(prefix))
                            // Ignore everything not starting with the right prefix
                            return
                        // Cut prefix
                        str = str.substr(prefix.length).trimRight()
                        if (!faStarted) {
                            // Wait for file annouuncement
                            if (str === "begin file announcement")
                                faStarted = true
                            return
                        }
                        if (!faFinished) {
                            // Process file annouuncement
                            if (str === "end file announcement") {
                                // Create elements for displaying announced files
                                for(i in outFileInfo) {
                                    outfiles.append('<span>'+i+'</span><br/>' )
                                    fi = outFileInfo[i]
                                    switch(fi.type) {
                                    case "image":
                                        fi.jq = $('<img src="' + "user/" + i + '" alt="' + i + '"/>')
                                            .css("width", fi.size.width)
                                            .css("height", fi.size.height)
                                            .addClass("outputFile")
                                            .appendTo(outfiles)
                                        outfiles.append("<br/>")
                                        break
                                    case "text":
                                        fi.jq = wrap('div').addClass("outputFile").appendTo(outfiles).html("Waiting...")
                                        break
                                    default:
                                        fi.jq = wrap('div').addClass("outputFile").appendTo(outfiles).html("UNKNOWN FILE TYPE")
                                    }
                                }
                                faFinished = true
                            }
                            else {
                                fi = JSON.parse(str)
                                outFileInfo[fi.name] = fi
                            }
                            return
                        }
                        m = str.match("file: (.*)")
                        if (m && m.length > 1) {
                            var name = m[1], svrname = "user/" + name
                            fi = outFileInfo[name]
                            switch(fi.type) {
                            case "image":
                                fi.jq.attr("src", svrname + "#" + new Date().getTime())
                                break
                            case "text":
                                // TODO: cut text, add download link
                                $.get(svrname).done(function(text) {
                                    text.replace("\r", "")
                                    var lines = text.split("\n")
                                        var table = wrap("table").appendTo(fi.jq.html(""))
                                        for (var i=0; i<lines.length; ++i) {
                                            var line = lines[i].split("\t")
                                            var row = wrap("tr").appendTo(table)
                                            for (var j=0; j<line.length; ++j)
                                                wrap("td").html(line[j]).appendTo(row)
                                        }
                                })
                                break
                            }
                            return
                        }
                        if (str === "sync") {
                            $.ajax("cmd/sync", {data: {cmd: syncToken}, type: "GET", cache: false})

                            // deBUG, TODO: Remove
                            .done(function(reply) {
                                var x = 1
                            })
                            .fail(function(error) {
                                var x = 1
                            })
                            return
                        }
                        if (str === "finished") {
                            equaresui.stopSimulation()
                            return
                        }
                        break
                    case 2:         // Report error
                        m = str.match(/^==\d+==> ERROR: line \d+: (.*)/)
                        if (m && m.length > 1) {
                            str = m[1].trimRight()
                            equaresui.stopSimulation()
                            // If possible, select box in problem
                            m = str.match(/\[box='(.*)'\]/)
                            if (m && m.length > 1)
                                schemeEditor.findBox(m[1]).select(true)
                        }
                        else {
                            m = str.match(/^==\d+==> (.*)/)
                            if (m && m.length > 1)
                                str = m[1].trimRight()
                        }
                        outfiles.css("text-align", "left").append('<span style="color: red">' + str + '</span><br/>')
                        break
                    }
                }
            })
            .fail(function(error){
                alert(error.responseText || error.statusText || ("Ajax error: " + error.status));
            });
    }

    function quickload() {
        beforeLoadScheme()
        $.get('cmd/quickload')
            .done(function(simulation) {
                simulation = JSON.parse(simulation)
                var obj = simulation.definition
                obj.name = simulation.name
                obj.description = simulation.description
                loadScheme(obj, false)
            })
            .fail(function() {
                alert('quickload failed')
                $("#loading-progress-overlay").hide()
            })
    }
    var saving = false
    function quicksave() {
        if (saving)
            return
        if (schemeEditor.modified) {
            saving = true
            var simulation = JSON.stringify({name: simProps.name, description: simProps.description, definition: schemeEditor.export()})
            $.post('cmd/quicksave', {simulation: simulation})
                .done(function() {
                    schemeEditor.modified = false
                })
                .fail(function(xhr) {
                    // Note: xhr.readyState==0 means we're doing post on page unload
                    if (xhr.readyState !== 0)
                        alert('quicksave failed')}
                )
                .always(function() {saving = false})
        }
    }

    quickload()

    setInterval(quicksave, 10000)

    $(window).unload(quicksave)

    var body = $('body')
    wrap('div').attr('id', 'before_login_action').hide().appendTo(body).click(quicksave)
    wrap('div').attr('id', 'after_login_action').hide().appendTo(body).click(quickload)
    wrap('div').attr('id', 'before_logout_action').hide().appendTo(body).click(quicksave)
    wrap('div').attr('id', 'after_logout_action').hide().appendTo(body).click(quickload)
}

})();
