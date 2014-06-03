var formatInfo = {}
formatInfo.init = function (elementId) {
    var myren = new marked.Renderer
    myren.link = function(href, title, text, external) {
        var foreign = href.match('://') && !href.match('^' + location.origin)
        if (arguments.length < 4)
            external = foreign
        var out = '<a href="' + href + '"'
        if (title)
          out += ' title="' + title + '"'
        if (foreign)
            out += ' class="external"'
        if (external)
            out += ' target="_blank"'
        out += '>'
        if (foreign)
            out += '<img src="/images/xlink.png"/>'
        out += text + '</a>';
        return out;
    }

    // Initialize MathJax
    MathJax.Hub.Config({
        elements: [elementId],
        TeX: {
            equationNumbers: { autoNumber: "AMS", useLabelIds: true},
        },
        tex2jax: {
            inlineMath: [['$','$']],
            processEscapes: true
        }
    })

    MathJax.Hub.Register.MessageHook("End Process", function (message) {
        var e = $('#' + elementId)
        e.html(marked(e.html(), {renderer: myren}))
    })

    function LatexEquationNumbersReset() {
        var ams = MathJax.Extension["TeX/AMSmath"]
        ams.startNumber = 0
        ams.labels = {}
    }
    formatInfo.update = function(data) {
        data = data.replace("http:///", location.origin+'/')
        var rx = /(^|\s+)(\?|=)\[([^\]]*)]\(([^)]*)\)/, rxg = new RegExp(rx.source, 'g')
        var m = data.match(rxg)
        if (m)
            for (var i=0; i<m.length; ++i) {
                var v = m[i], mm = data.match(rx)
                var a = myren.link(mm[4], null, mm[3], mm[2] === '?')
                data = data.replace(v, mm[1] + a)
            }
        var e = $('#' + elementId)
        e.html(data)
        MathJax.Hub.Queue(LatexEquationNumbersReset, ["Typeset", MathJax.Hub, e[0]])
    }

    marked.setOptions({
        gfm: true
    })
}
