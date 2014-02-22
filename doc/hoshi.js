//
//  Hoshi.js                                                               
//  --------                                                               
//                                                                         
//  Javascript code to implement the Hoshi instructions web page. We're    
//  going to write the documentation as a web page at least partly as  
//  a vehicle for me to learn the technology (I've never done any web      
//  programming). Hopefully the documentation will be usable, but you're   
//  likely to find a lot of unusual stuff here. Please be merciful.         
//                                                                         
//  First we have some utilities that I think might be useful to me in the 
//  future. We'll want to show trees in several contexts here so we'll     
//  start with a tree layout utility. The original version of this is due  
//  to Walker. It was later improved by Buchheim, et al.                   
//

//
//  TreeUtil                                                            
//  --------                                                            
//                                                                      
//  This is a set of tree handling facilities for displaying trees in a 
//  canvas.                                                             
//

var TreeUtil = (function invocation() {

    //
    //  TreeNode                                                         
    //  --------                                                         
    //                                                                   
    //  This and its children represent a subtree. If the node is the root 
    //  it represents the tree.                                          
    //

    var TreeNode = (function invocation() {
        
        var label = null;
        var parent = null;
        var number = -1;
        var children = [];

        var prelim = 0;
        var x = 0;
        var y = 0;
        var thread = null;
        var mod = 0;
        var ancestor = null;
        var change = 0;
        var shift = 0;

        return function(label, parent, number, children) {

            if (arguments.length > 0) {
                this.label = label;
            } else {
                this.label = null;
            }

            if (arguments.length > 1) {
                this.parent = parent;
            } else {
                this.parent = null;
            }

            if (arguments.length > 2) {
                this.number = number;
            } else {
                this.number = -1;
            }

            if (arguments.length > 3) {
                this.children = children;
            } else {
                this.children = [];
            }

            this.prelim = 0;
            this.x = -1;
            this.y = -1;
            this.thread = null;
            this.mod = 0;
            this.ancestor = this;
            this.change = 0;
            this.shift = 0;

        };

    })();

    //
    //  dumpTree                 
    //  --------                 
    //                           
    //  Dump tree for debugging. 
    //

    function dumpTree(tree, indent) {
     
        if (arguments.length < 2) {
            indent = "";
        }

        console.log(indent + "node " + tree.label + " " + tree.x + " " + tree.y);
        for (var i = 0; i < tree.children.length; i++) {
            dumpTree(tree.children[i], indent + "    ");
        }

    }

    //
    //  Buchheim's Algorithm                                              
    //  --------------------                                              
    //                                                                    
    //  The next few functions implement Buchheim's algorithm to layout a 
    //  tree on a plane.                                                  
    //                                                                    
    //  Ok, I can't take credit for this. I first transcribed it from a   
    //  python version at billmill.org. But there were parts that weren't 
    //  clear (or I doubted), so I went back to the original Buchheim, et 
    //  al article and updated from the pseudo-code there.                
    //

    function firstWalk(v) {

        var distance = 1;

        if (v.children.length == 0) {

            if (leftSibling(v) == null) {
                v.prelim = 0;
            } else {
                v.prelim = leftSibling(v).prelim + distance;
            }

        } else {

            var defaultAncestor = v.children[0];
            for (var i = 0; i < v.children.length; i++) {
                var child = v.children[i];
                firstWalk(child);
                defaultAncestor = apportion(child, defaultAncestor);
            }

            executeShifts(v);

            var midpoint = (v.children[0].prelim +
                            v.children[v.children.length - 1].prelim) / 2;

            var w = leftSibling(v);
            if (w != null) {
                v.prelim = w.prelim + distance;
                v.mod = v.prelim - midpoint;
            } else {
                v.prelim = midpoint;
            }

        }

    }

    function leftSibling(v) {

        if (v.parent == null || v.number == 0) {
            return null;
        }

        return v.parent.children[v.number - 1];

    }

    function apportion(v, defaultAncestor) {

        var distance = 1;

        w = leftSibling(v);
        if (w != null) {

            var vir = v;
            var vor = v;
            var vil = w;
            var vol = leftmostSibling(vir);
            var sir = vir.mod;
            var sor = vor.mod;
            var sil = vil.mod;
            var sol = vol.mod;

            while (nextRight(vil) != null && nextLeft(vir) != null) {

                vil = nextRight(vil);
                vir = nextLeft(vir);
                vol = nextLeft(vol);
                vor = nextRight(vor);
                vor.ancestor = v;
                var shift = (vil.prelim + sil) - (vir.prelim + sir) + distance;

                if (shift > 0) {
                    moveSubtree(ancestor(vil, v, defaultAncestor), v, shift);
                    sir = sir + shift;
                    sor = sor + shift;
                }

                sil += vil.mod;
                sir += vir.mod;
                sol += vol.mod;
                sor += vor.mod;

            }

            if (nextRight(vil) != null && nextRight(vor) == null) {

                vor.thread = nextRight(vil);
                vor.mod += sil - sor;

            }

            if (nextLeft(vir) != null && nextLeft(vol) == null) {

                vol.thread = nextLeft(vir);
                vol.mod += sir - sol;
                defaultAncestor = v

            }

        }

        return defaultAncestor

    }

    function leftmostSibling(v) {

        if (v.parent != null && v.number > 0) {
            return v.parent.children[0];
        }

        return null;

    }

    function nextLeft(v) {

        if (v.children.length > 0) {
            return v.children[0];
        }

        return v.thread;

    }

    function nextRight(v) {

        if (v.children.length > 0) {
            return v.children[v.children.length - 1];
        }

        return v.thread;

    }

    function moveSubtree(wl, wr, shift) {

        var subtrees = wr.number - wl.number;
        wr.change -= shift / subtrees;
        wr.shift += shift;
        wl.change += shift / subtrees;
        wr.prelim += shift;
        wr.mod += shift;

    }

    function executeShifts(v) {

        var shift = 0;
        var change = 0;

        for (var i = v.children.length - 1; i >= 0; i--) {
            var child = v.children[i];
            child.prelim += shift;
            child.mod += shift;
            change += child.change;
            shift += child.shift + change;
        }

    }

    function ancestor(vil, v, defaultAncestor) {

        if (v.parent == null) {
            return null;
        }

        for (var i = 0; i < v.parent.children.length; i++) {

            if (vil.ancestor == v.parent.children[i]) {
                return vil.ancestor;
            }

        }

        return defaultAncestor;

    }

    function secondWalk(v, m, depth) {

        v.x = v.prelim + m;
        v.y = depth;

        for (var i = 0; i < v.children.length; i++) {
            secondWalk(v.children[i], m + v.mod, depth + 1);
        }

    }

    //
    //  updateBoundingBox                                             
    //  -----------------                                             
    //                                                                  
    //  Update a bounding box with the bounding box of a tree. If       
    //  originally called with nulls the tree will be the only thing in 
    //  the bounding box.                                               
    //

    function updateBoundingBox(tree, boundingBox) {
        
        if (boundingBox[0] == null || tree.x < boundingBox[0]) {
            boundingBox[0] = tree.x;
        }

        if (boundingBox[2] == null || tree.x > boundingBox[2]) {
            boundingBox[2] = tree.x;
        }

        if (boundingBox[1] == null || tree.y < boundingBox[1]) {
            boundingBox[1] = tree.y;
        }

        if (boundingBox[3] == null || tree.y > boundingBox[3]) {
            boundingBox[3] = tree.y;
        }

        for (var i = 0; i < tree.children.length; i++) {
            updateBoundingBox(tree.children[i], boundingBox);
        }

    }

    //
    //  adjustTreeCoordinates                         
    //  ---------------------                         
    //                                                  
    //  Scale and shift a tree to fit in a desired box. 
    //

    function adjustTreeCoordinates(tree, xscale, yscale, xoffset, yoffset) {

        tree.x = tree.x * xscale + xoffset;
        tree.y = tree.y * yscale + yoffset;

        for (var i = 0; i < tree.children.length; i++) {
            adjustTreeCoordinates(tree.children[i], xscale, yscale, xoffset, yoffset);
        }

    }

    //
    //  drawTreeEdges                                                 
    //  -------------                                                 
    //                                                                  
    //  Draw all the edges of the tree. In a later pass we'll label the 
    //  nodes.                                                          
    //

    function drawTreeEdges(tree, ctx, isRoot, options) {

        if (arguments.length < 3) {
            isRoot = false;
        }

        if (!isRoot) {

            ctx.linewidth = 4;
            ctx.beginPath();
            ctx.moveTo(tree.x, tree.y);
            ctx.lineTo(tree.parent.x, tree.parent.y);
            ctx.stroke();

        }

        for (var i = 0; i < tree.children.length; i++) {
            drawTreeEdges(tree.children[i], ctx, false, options);
        }

    }

    //
    //  labelTreeNodes                               
    //  --------------                               
    //                                           
    //  Walk a tree labelling each of the nodes. 
    //

    function labelTreeNodes(tree, ctx, drawNode) {

        drawNode(ctx, tree.x, tree.y, tree.label);

        for (var i = 0; i < tree.children.length; i++) {
            labelTreeNodes(tree.children[i], ctx, drawNode);
        }

    }

    //
    //  drawNode                                                          
    //  --------                                                          
    //                                                                     
    //  Draw a node label with the provided location, context and font.    
    //  Assume the links have already been drawn. Clear a bit of space and 
    //  draw the label.                                                    
    //

    function drawNode(ctx, x, y, label, options) {

        if (options.font) {
            ctx.font = options.font;
        }

        var rscale = 1.0;

        if (options.scale) {
            rscale = options.scale;
        }

        var style = "circle";
        if (options.style) {
            style = options.style;
        }

        var nodeStyles = [];
        if (options.nodeStyles) {
            nodeStyles = options.nodeStyles;
        }

        for (i = 0; i < nodeStyles.length; i++) {

            if (label.match(nodeStyles[i][0])) {    

                if (nodeStyles[i][1].jfont) {
                    ctx.font = nodeStyles[i][1].jfont;
                }

                if (nodeStyles[i][1].style) {
                    style = nodeStyles[i][1].style;
                }

                break;

            }

        }

        var width = ctx.measureText(label).width;
        var height = ctx.measureText("M").width * 1.0;

        ctx.beginPath();

        if (style == "open") {
            if (options["background"]) {
                ctx.strokeStyle = options["background"];
            } else {
                ctx.strokeStyle = "rgba(100%, 100%, 100%, 1)";
            }
        } else {
            ctx.strokeStyle = "rgba(0%, 0%, 0%, 1)";
        }

        ctx.arc(x, y, height * rscale, 0, Math.PI * 2, true);
        ctx.closePath();

        if (options["background"]) {
            ctx.fillStyle = options["background"];
        } else {
            ctx.fillStyle = "rgba(100%, 100%, 100%, 1)";
        }

        ctx.fill();
        ctx.stroke();

        ctx.fillStyle = "rgba(0%, 0%, 0%, 1)";
        ctx.fillText(label, x - width / 2, y + height / 2);

    }

    //
    //  buildTree                                                       
    //  ---------                                                       
    //                                                                   
    //  It's easiest for clients to pass trees as nested arrays but we   
    //  need a bunch of connected objects. This converts from the easier 
    //  literal form to an internal form.                                
    //

    function buildTree(tuple) {

        var node = new TreeNode(tuple[0]);

        for (var i = 1; i < tuple.length; i++) {
            var child = buildTree(tuple[i]);
            child.parent = node;
            child.number = i - 1;
            node.children.push(child);
        }

        return node;

    }

    //
    //  drawForest                        
    //  ----------                        
    //                                     
    //  Draw a forest in the bounding box. 
    //

    function drawForest(forest, canvas, boundingBox, drawNode, options) {

        //
        //  The forest is just a list of trees. For the layout algorithm 
        //  to work we need a single tree. We just push a label on the   
        //  front of the forest.                                         
        //

        var tree = buildTree(["bogus"].concat(forest));

        //
        //  Run Buchheim's algorithm to layout the tree. 
        //

        firstWalk(tree);
        secondWalk(tree, -tree.prelim, 1);

        //
        //  Try to scale the bounding box to the desired size. This       
        //  doesn't account for the space used to label the nodes so will 
        //  take a bit of manual tweaking.                                
        //

        var drawnBb = [null, null, null, null];

        for (var i = 0; i < tree.children.length; i++) {
            updateBoundingBox(tree.children[i], drawnBb);
        }

        var minSpan = -1;
        if (options.minSpan) {
            minSpan = options.minSpan;
        }

        var xscale = (boundingBox[2] - boundingBox[0]) /
                     Math.max((drawnBb[2] - drawnBb[0]), minSpan);

        if (drawnBb[2] == drawnBb[0]) {
            xscale = 1.0;
        }

        var minDepth = -1;
        if (options.minDepth) {
            minDepth = options.minDepth;
        }

        var yscale = (boundingBox[3] - boundingBox[1]) /
                     Math.max((drawnBb[3] - drawnBb[1]), minDepth);

        if (drawnBb[3] == drawnBb[1]) {
            yscale = 1.0;
        }

        var xoffset = (boundingBox[2] + boundingBox[0]) / 2.0 -
                      (drawnBb[2] + drawnBb[0]) / 2.0 * xscale;
        var yoffset = boundingBox[1] - drawnBb[1] * yscale;

        adjustTreeCoordinates(tree, xscale, yscale, xoffset, yoffset);

        //
        //  Draw and label the tree. 
        //

        var ctx = canvas.getContext("2d");

        for (var i = 0; i < tree.children.length; i++) {
            drawTreeEdges(tree.children[i], ctx, true);
        }

        for (var i = 0; i < tree.children.length; i++) {
            labelTreeNodes(tree.children[i], ctx, drawNode);
        }

    }

    //
    //  TreeUtil                    
    //  --------                    
    //                              
    //  Return the final namespace. 
    //

    return {

        //
        //  Public symbols should be very simple, generally redirecting to 
        //  a private symbol.                                              
        //

        drawForest: function(forest, canvas, boundingBox, drawNode, options) {
            if (arguments.length < 5) {
                options = {};
            }
            return drawForest(forest, canvas, boundingBox, drawNode, options);
        },
        
        drawNode: function(options) {
            if (arguments.length < 1) {
                options = {};
            }
            return function(ctx, x, y, label) { drawNode(ctx, x, y, label, options); };
        },

        buildTree: function(tuple) {
           return buildTree(tuple);
        },

        dumpTree: function(tuple) {
           return dumpTree(tuple);
        },

    };

})();

//
//  Hoshi.js                                                        
//  --------                                                        
//                                                                  
//  This Javascript file handles the post-load processing (and      
//  interactivity stuff) for our Hoshi documentation. Everything is 
//  buried in the window loading callback.                          
//

window.onload = function() {

    //
    //  createBNFGrammar                   
    //  ----------------                   
    //                                     
    //  Conveniently format a BNF grammar. 
    //

    function createBNFGrammar(script) {

        var nodeStyles =
        [
            [/[a-zA-Z]+/, {font: "i", style: "open", jfont: "italic 12pt serif"}],
            [/.*/,        {font: "rm", style: "open", jfont: "12pt serif"}],
        ];
        
        //
        //  Parse the text of the script. We should have options that look 
        //  like this:                                                     
        //                                                                 
        //  :: key value                                                   
        //                                                                 
        //  The rest should be a list of rules.
        //

        var lineList = script.text.split(/\n/);
        var ruleList = new Array();
        var options = {};

        for (var i = 0; i < lineList.length; i++) {
          
            line = lineList[i];

            if (line.substring(0, 2) == "::") {
                words = line.split(/\s+/);
                options[words[1]] = words[2];
                continue;
            }
            
            if (line.trim() == "") {
                continue;
            }

            ruleList[ruleList.length] = line;

        }

        var classRoot = "bnf_grammar";
        if (options["class"]) {
            classRoot = options["class"];
        }

        //
        //  Start laying out the widget. 
        //

        var table = document.createElement("table");
        table.className = classRoot;

        for (var i = 0; i < ruleList.length; i++) {

            var ruleText = ruleList[i];
            var tr = document.createElement("tr");

            var symbols = ruleText.split(/\s+/);

            while (symbols.length > 0 && symbols[0].match(/^\s*$/)) {
                symbols.shift();
            }
      
            while (symbols.length > 0 && symbols[symbols.length - 1].match(/^\s*$/)) {
                symbols.pop();
            }

            var nextSymbol = 0;

            if (symbols[nextSymbol] == '|') {

                var td = document.createElement("td");
                tr.appendChild(td);

                td = document.createElement("td");
                td.className = "center";
                td.innerHTML = "&#124"
                tr.appendChild(td);

                nextSymbol++;

            }

            var td = document.createElement("td");
            var html = "";

            for (var j = nextSymbol; j < symbols.length; j++) {

                if (symbols[j] == "::=") {

                    td.innerHTML = html.trim();
                    td.className = "left";
                    html = "";
                    tr.appendChild(td);

                    td = document.createElement("td");
                    td.className = "center";
                    td.innerHTML = "&rarr;";
                    tr.appendChild(td);
                    td = document.createElement("td");

                    continue;

                }

                if (symbols[j] == "::==") {

                    td.innerHTML = html.trim();
                    td.className = "left";
                    html = "";
                    tr.appendChild(td);

                    td = document.createElement("td");
                    td.className = "center";
                    td.innerHTML = "&rArr;";
                    tr.appendChild(td);
                    td = document.createElement("td");

                    continue;

                }

                if (symbols[j] == '|') {
                    html += "&emsp;&#124;&emsp;"
                    continue;
                }

                for (k = 0; k < nodeStyles.length; k++) {

                    if (symbols[j].match(nodeStyles[k][0])) {    

                        if (nodeStyles[k][1].font == "rm") {
                            html += " " + symbols[j];
                            break;
                        }

                        html += " <" + nodeStyles[k][1].font + ">" +
                                symbols[j] +
                                "</" + nodeStyles[k][1].font + ">";

                        break;

                    }

                }

            }

            td.innerHTML = html.trim();
            td.className = "left";
            tr.appendChild(td);

            table.appendChild(tr);

        }

        script.parentNode.replaceChild(table, script);

    }

    //
    //  createTreeDiagram                                                  
    //  -----------------                                                  
    //                                                                     
    //  Create a canvas with a simple tree diagram. This is convenient for 
    //  parse trees.                                                       
    //

    function createTreeDiagram(script) {

        //
        //  Parse the text of the script. We should have options that look 
        //  like this:                                                     
        //                                                                 
        //  :: key value                                                   
        //                                                                 
        //  The rest should be a Javascript array.                         
        //

        var lineList = script.text.split(/\n/);
        var jsExpr = "";
        var options = {};

        for (var i = 0; i < lineList.length; i++) {
          
            line = lineList[i];

            if (line.substring(0, 2) == "::") {
                words = line.split(/\s+/);
                options[words[1]] = words[2];
                continue;
            }
            
            jsExpr = jsExpr + "\n" + line;

        }

        var classRoot = "tree_diagram";
        if (options["class"]) {
            classRoot = options["class"];
        }

        var margin = 20;
        if (options["margin"]) {
            margin = parseInt(options["margin"]);
        }

        //
        //  Start laying out the diagram.
        //

        var treeTable = eval(jsExpr)[0];

        var canvas = document.createElement("canvas");
        canvas.className = classRoot;
        script.parentNode.replaceChild(canvas, script);

        if (options["width"]) {
            canvas.width = options["width"];
        } else {
            canvas.width = 700;
        }

        if (options["height"]) {
            canvas.height = options["height"];
        } else {
            canvas.height = 600;
        }

        TreeUtil.drawForest(treeTable.trees,
                            canvas,
                            [margin, margin, canvas.width - margin, canvas.height - margin],
                            TreeUtil.drawNode({
                                font: "12pt serif",
                                scale: 1.1,
                                style: "open",
                                background: window.getComputedStyle(canvas).backgroundColor,
                                nodeStyles: treeTable.nodeStyles,
                            }),
                            { minSpan: -1, minDepth: -1});

    }

    //
    //  createTreeBuildingWidget                                  
    //  ------------------------                                  
    //                                                            
    //  Create a widget that allows the user to step forwards and 
    //  backwards through parse tree building.                    
    //

    function createTreeBuildingWidget(script) {

        var derivationTable;
        var step;
        var forestCanvas;
        var ruleTd;
        var margin;

        //
        //  drawDerivationStep                                             
        //  ------------------                                             
        //                                                                 
        //  Draw a single step in the derivation. We're going to call this 
        //  on button clicks after adjusting 'step'.                       
        //

        function drawDerivationStep() {

            //
            //  Adjust the derivation step. 
            //

            if (arguments.length > 0) {

                if (arguments[0] == '<<') {
                    step = 0;
                } else if (arguments[0] == "<" && step > 0) {
                    step--;
                } else if (arguments[0] == ">" &&
                           step < derivationTable.derivation.length - 1) {
                    step++;
                } else if (arguments[0] == '>>') {
                    step = derivationTable.derivation.length - 1;
                }

            }
                
            //
            //  Format the rule and set the table cell. 
            //

            var html = "<b>&ensp;Last Rule: </b>";
            var symbols = derivationTable.derivation[step].rule.split(/\s+/);

            for (var i = 0; i < symbols.length; i++) {

                if (symbols[i] == "::=") {
                    html += " &larr;";
                    continue;
                }

                var nodeStyles = derivationTable.nodeStyles;

                for (j = 0; j < nodeStyles.length; j++) {

                    if (symbols[i].match(nodeStyles[j][0])) {    

                        if (nodeStyles[j][1].font == "rm") {
                            html += " " + symbols[i];
                            break;
                        }

                        html += " <" + nodeStyles[j][1].font + ">" +
                                symbols[i] +
                                "</" + nodeStyles[j][1].font + ">";

                        break;

                    }

                }

            }

            ruleTd.innerHTML = html;

            //
            //  Draw the forest. 
            //

            forestCanvas.width = forestCanvas.width;

            TreeUtil.drawForest(derivationTable.derivation[step].trees,
                                forestCanvas,
                                [margin, margin, forestCanvas.width - margin, forestCanvas.height - margin],
                                TreeUtil.drawNode({
                                    font: "10pt serif",
                                    scale: 1.1,
                                    style: "open",
                                    background: window.getComputedStyle(forestCanvas).backgroundColor,
                                    nodeStyles: derivationTable.nodeStyles,
                                }),
                                { minSpan: 8, minDepth: 5});

        }

        //
        //  Parse the text of the script. We should have options that look 
        //  like this:                                                     
        //                                                                 
        //  :: key value                                                   
        //                                                                 
        //  The rest should be a Javascript list with the map as first element.
        //

        var lineList = script.text.split(/\n/);
        var jsExpr = "";
        var options = {};

        for (var i = 0; i < lineList.length; i++) {
          
            line = lineList[i];

            if (line.substring(0, 2) == "::") {
                words = line.split(/\s+/);
                options[words[1]] = words[2];
                continue;
            }
            
            jsExpr = jsExpr + "\n" + line;

        }

        derivationTable = eval(jsExpr)[0];
        step = 0;

        var classRoot = "tree_building_widget";
        if (options["class"]) {
            classRoot = options["class"];
        }

        margin = 20;
        if (options["margin"]) {
            margin = parseInt(options["margin"]);
        }

        //
        //  Start laying out the widget. 
        //

        var table = document.createElement("table");
        table.className = classRoot;
        var tr = document.createElement("tr");
        tr.className = classRoot;
        var td = document.createElement("td");
        td.className = classRoot;
      
        td.colSpan = 2;

        forestCanvas = document.createElement("canvas");
        forestCanvas.className = classRoot;
        forestCanvas.width = 720;
        forestCanvas.height = 600;
        td.appendChild(forestCanvas);
        tr.appendChild(td);
        table.appendChild(tr);

        var tr = document.createElement("tr");
        tr.className = classRoot;
        var ruleTd = document.createElement("td");
        ruleTd.className = classRoot + "_rule";
        ruleTd.innerHTML = "";
        var td = document.createElement("td");
        td.className = classRoot + "_buttons";

        buttonList = [
            { action:"<<", label:"&vert;&vrtri;" },
            { action:"<", label:"&vrtri;" },
            { action:">", label:"&vltri;" },
            { action:">>", label:"&vltri;&vert;" },
        ];

        for (var i = 0; i < buttonList.length; i++) {

            var button = document.createElement("button");
            button.innerHTML = "<b>" + buttonList[i]["label"] + "</b>";

            button.onclick = (function(action) {
                return function() {
                    drawDerivationStep(action);
                }
            })(buttonList[i]["action"]);

            td.appendChild(button);
            var thinSpace = document.createElement("span");
            thinSpace.innerHTML = "&nbsp";
            td.appendChild(thinSpace);

        }

        tr.appendChild(ruleTd);
        tr.appendChild(td);
        table.appendChild(tr);

        drawDerivationStep();
        script.parentNode.replaceChild(table, script);

    }

    //
    //  createCodeSample                                                
    //  ----------------                                                
    //                                                                  
    //  Create a code sample widget. Hoshi supports multiple client     
    //  languages and we generally want to provide a single widget for  
    //  each sample where the reader can switch among client languages. 
    //

    function createCodeSample(script) {
    
        //
        //  Parse the text of the script. We should have a sequence of 
        //  code samples by language with delimiters that look like    
        //                                                             
        //  !! <language>                                          
        //                                                             
        //  We split this into a map from language to sample.          
        //

        var codeList = new Array();
        var language = "";
        var text = new Array();
        var options = {};

        var lineList = script.text.split(/\n/);
        lineList[lineList.length] = "!!";

        for (var i = 0; i < lineList.length; i++) {
          
            line = lineList[i];

            if (line.substring(0, 2) == "::") {

                words = line.split(/\s+/);
                options[words[1]] = words[2];
                continue;

            }
            
            if (line.substring(0, 2) == "!!") {

                while (text.length > 0 && text[0].match(/^\s*$/)) {
                    text.shift();
                }
          
                while (text.length > 0 && text[text.length - 1].match(/^\s*$/)) {
                    text.pop();
                }
              
                if (text.length > 0) {
                    codeList[codeList.length] = { language: language,
                                                  text: text.join("\n") };
                }

                language = line.split(/\s+/)[1];
                text = new Array();

                continue;

            }
            
            line = line.replace("&", "&amp;");
            line = line.replace("<", "&lt;");
            line = line.replace(">", "&gt;");

            text[text.length] = line;

        }

        var classRoot = "code_sample";
        if (options["class"]) {
            classRoot = options["class"];
        }

        //
        //  Start laying out the widget. 
        //

        var table = document.createElement("table");
        table.className = classRoot;
        var tr = document.createElement("tr");
        tr.className = classRoot;
        var td = document.createElement("td");
        td.className = classRoot + "_code";
      
        if (codeList.length > 1) {
            td.colSpan = 2;
        }

        var codeDiv = document.createElement("div");
        codeDiv.className = classRoot;
        codeDiv.innerHTML = "<code><pre>" + codeList[0].text + "</pre></code>";
        td.appendChild(codeDiv);
        tr.appendChild(td);
        table.appendChild(tr);

        if (codeList.length > 1) {

            var tr = document.createElement("tr");
            var codeTd = document.createElement("td");
            codeTd.className = classRoot + "_language";
            codeTd.innerHTML = "language";
            var td = document.createElement("td");
            td.className = classRoot + "_buttons";
            codeTd.innerHTML = "<b>" + codeList[0].language + "</b>";    

            for (var i = 0; i < codeList.length; i++) {

                var button = document.createElement("button");
                button.innerHTML = codeList[i].language;

                button.onclick = (function(td, language, div, text) {
                    return function() {
                        td.innerHTML = "<b>" + language + "</b>";    
                        div.innerHTML = "<code><pre>" + text + "</pre></code>";
                    }
                })(codeTd, codeList[i].language, codeDiv, codeList[i].text);

                td.appendChild(button);

            }

            tr.appendChild(codeTd);
            tr.appendChild(td);
            table.appendChild(tr);

        }

        script.parentNode.replaceChild(table, script);

    }

    //
    //  Scripts are the simplest way to embed text that won't be parsed    
    //  (so we don't have to worry about '<' and '&'). We'll use that fact 
    //  to encode stuff we can create after loading.                       
    //

    for (;;) {

        var scriptList = document.getElementsByTagName("script");
        for (var i = scriptList.length - 1; i > 0; i++) {

            script = scriptList[i];
        
            if (script.type == "text/BNFGrammar") {
                createBNFGrammar(script);
            } else if (script.type == "text/TreeDiagram") {
                createTreeDiagram(script);
            } else if (script.type == "text/TreeBuildingWidget") {
                createTreeBuildingWidget(script);
            } else if (script.type == "text/CodeSample") {
                createCodeSample(script);
            } else {
                continue;
            }

            break;

        }

        if (i == 0) {
            break;
        }

    }

};

