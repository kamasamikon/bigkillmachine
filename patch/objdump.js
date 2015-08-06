
function strobj(o) {
    if (!o) 
        return 'null';

    if (typeof (o) == "object") {
        if (!strobj.chk) 
            strobj.chk = new Array();
        for (var i = 0, k = strobj.chk.length; i < k; ++i) {
            if (strobj.chk[i] == o) {
                return '{}';
            }
        }
        strobj.chk.push(o);
    }

    var k = "", na = typeof (o.length) == "undefined" ? 1 : 0, s = "";
    for (var p in o) {
        if (na) 
            k = "'" + p + "':";

        if (typeof o[p] == "string") 
            s += k + "'" + o[p] + "',";
        else if (typeof o[p] == "object") 
            s += k + strobj(o[p]) + ",";
        else 
            s += k + o[p] + ",";
    }

    if (typeof (o) == "object") 
        strobj.chk.pop();

    if (na) 
        return "{" + s.slice(0, -1) + "}";
    else 
        return "[" + s.slice(0, -1) + "]";
}


function dumpObj(obj, name, indent, depth) {
    if (depth > 10) {
        return indent + name + ": <Maximum Depth Reached>\n";
    }

    if (typeof obj == "object") {
        var child = null;
        var output = indent + name + "\n";

        indent += "\t";
        for (var item in obj)
        {
            try {
                child = obj[item];
            } catch (e) {
                child = "<Unable to Evaluate>";
            }
            if (typeof child == "object") {
                output += dumpObj(child, item, indent, depth + 1);
            } else {
                output += indent + item + ": " + child + "\n";
            }
        }
        return output;
    } else {
        return obj;
    }
}

function showbt() {
    try {
        shit.shit.shit = 'sd';
    }
    catch (e) {
        var stack = e.stack.replace(/^[^\(]+?[\n$]/gm, '')
            .replace(/^\s+at\s+/gm, '')
            .replace(/^Object.<anonymous>\s*\(/gm, '{anonymous}()@')
            .split('\n');

        XCOM.dalog("e", "BT:" + e.stack);
        alert(e.stack);
    }
}

