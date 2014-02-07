$(document).ready(function() {
    var rootLayout = ctmDock.makeRoot().setLayout( {
        type: "vertical",
        tools: false
    } );
    var c1, c2, c3;
    c1 = rootLayout.add( {height: 35, nosplit: true} );
    c2 = rootLayout.add( {nosplit: true} );
    c2.master = true;
    c3 = rootLayout.add( {height: 15, nosplit: true} );
    var header = $(c1.dom)
    header.attr("id", "header").html('<img id="projectlogo" src="pix/logo-small.png" title="logo"/><div id="projectname">Equares</div>');

    // Create main menu
    function wrap(tag) { return $("<" + tag + "></" + tag + ">") }
    function createMenu(root, items, depth) {
        if (depth === undefined)
            depth = 0
        var m = wrap("ul").appendTo(root)
        m.addClass("menu-depth-"+depth)
        function h0() {}
        for (var i=0; i<items.length; ++i) {
            var item = items[i]
            var li = wrap("li").appendTo(m)
            var a = wrap("a").html(item.text).attr("href", "#").appendTo(li)
            ;(function(){
                var h = item.handler instanceof Function? item.handler: h0
                a.click(function(e) {
                    e.preventDefault()
                    h()
                })
            })()
            if (item.menu)
                createMenu(li, item.menu, depth+1)
        }
        return m
    }
    var mainMenu = createMenu(wrap("div").appendTo(header), [
        {text: "Go", menu: [
            {text: "Simulation editor", handler: function() { equaresui.setSceneSource.call(c2) }}
        ]},
        {text: "Simulation", menu: [
            {text: "Examples", menu: [
                {text: "Simple pendulum, phase trajectory", handler: function() { equaresui.loadExample("simple-pendulum-1") }},
                {text: "Simple pendulum, several plots", handler: function() { equaresui.loadExample("simple-pendulum-2") }},
                {text: "Double pendulum, trajectory projection", handler: function() { equaresui.loadExample("double-pendulum-t") }},
                {text: "Double pendulum, Poincare map", handler: function() { equaresui.loadExample("double-pendulum-psec") }}
            ]},
            {text: "Open", handler: function() { $("#open-scheme-file-dialog").dialog("open") } },
            {text: "Save", handler: function() { $("#save-scheme-file-dialog").dialog("open") } },
            {text: "Run", handler: function() { equaresui.runScheme() } }
        ]}
    ]).attr("id", "mainmenu").menu()

    // Nested menu positioning does not work as expected, TODO
    /*
    mainMenu.menu("option", "position", { my:'left top', at:'left bottom' })
    mainMenu.find(".menu-depth-1").menu().menu("option", "position", { my:'left top', at:'right top' })
    */
    mainMenu.menu("option", "position", { my:'left top', at:'left+30 bottom' })

    // Create dialogs
    function updateOpenState() {
        var f = $("#open-scheme-file-dialog :file")[0]
        var o = $(".ui-dialog-buttonpane button:contains('Open')")
        o.button(f.files.length>0? "enable": "disable")
    }
    $("#open-scheme-file-dialog :file").change(updateOpenState)

    $("#open-scheme-file-dialog").dialog({
        autoOpen: false,
        modal: true,
        width: 400,
        buttons: {
            Open: function() {
                equaresui.openScheme($("#open-scheme-file")[0].files[0])
                $(this).dialog("close")
            },
            Cancel: function() { $(this).dialog("close") }
        },
        open: function() {
                //$(this).children("form")[0].reset()
                updateOpenState()
            }
    })
    $("#save-scheme-file-dialog").dialog({
        autoOpen: false,
        modal: true,
        buttons: {
            Save: function() {
                equaresui.saveScheme($("#save-scheme-file-name").val())
                $(this).dialog("close")
            },
            Cancel: function() { $(this).dialog("close") }
        }
    })

    $("#running-simulation").dialog({
        autoOpen: false,
        modal: true,
        width: 800,
        close: function() { equaresui.stopSimulation() },
        buttons: {
            Stop: function() { equaresui.stopSimulation() },
            Close: function() { $(this).dialog("close") }
        }
    })

    $(c2.dom).addClass("mymain");
    $(c3.dom).addClass("myfooter").html('<a target="_blank" href="http://ctmech.ru/">Computer Technologies in Engineering</a>');
    rootLayout.resize();    // Because borders have changed

    // Set scene source by default
    equaresui.setSceneSource.call(c2)
});
