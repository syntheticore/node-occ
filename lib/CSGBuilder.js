
var occ = require("../");
var should = require('should');

var doDebug = true;

var CSG_Executer = function (debug)
{
    this.map = {};
    this.csg_tree = {};

    doDebug = debug || false;

};

CSG_Executer.prototype.executeCSG = function (csg_tree)
{
    this.csg_tree = csg_tree;
    this.map = {};
    if (!csg_tree.hasOwnProperty("root")) {
        throw "csg_tree has no root";
    }
    var rootOp = csg_tree.root;

    return this.executeCSGCommand(rootOp,csg_tree[rootOp]);
};

CSG_Executer.prototype.resolveArguments = function(myArguments)
{
    var resolved_arguments = [];
    var arg;
    if (doDebug) { console.log(" arguments = ",myArguments); }
    for (var i in myArguments) {
        arg = myArguments[i];

        if (doDebug) { console.log(" arguments = ",arg); }
        if (typeof arg ==="string") {
            var name = arg;
            var shape;
            if (this.map.hasOwnProperty(name)) {
                shape = this.map[name];
            } else {
                // resolve the name
                if (!this.csg_tree.hasOwnProperty(name)) {
                    throw "CSG_Executer.resolveArguments: cannot find definition of " + name + " in CSG tree";
                } else {
                    shape = this.executeCSGCommand(name,this.csg_tree[name]);
                }
            }
            resolved_arguments.push(shape);

        } else {
            resolved_arguments.push(arg);
        }
    }
    return resolved_arguments;
};

CSG_Executer.prototype.executeCSGCommand = function (name, op)
{
    var method = op.type;
    var parameters = this.resolveArguments(op.parameters);

    if (doDebug) {
        console.log(" executing command : " + method + " with ",parameters );
    }
    if (!occ[method]) {
        throw "CSG_Executer.executeCSGCommand: cannot find method '"+ method + "' on occ object";
    }
    var obj = occ[method].apply(occ, parameters);

    if (doDebug) {
        obj.numFaces.should.be.greaterThan(0);
    }
    this.map[name] =obj;
    return obj;
};

exports.sample_csg_tree = {

    c1:      { type: "makeCylinder" , parameters: [ [-20.0,  0.0, 0.0] ,  [ 20.0, 0.0, 0.0] , 5.0]  },
    c2:      { type: "makeCylinder" , parameters: [  [  0.0,-20.0, 0.0] , [  0.0,20.0, 0.0] , 5.0] },
    c3:      { type: "makeCylinder" , parameters: [  [  0.0,  0.0,-20.0] , [  0.0,0.0, 20.0] ,5.0] },
    c4:      { type: "fuse" ,         parameters: [ "c1", "c2" ] },
    cross:   { type: "fuse" ,         parameters: [ "c4", "c3" ] },
    box:     { type: "makeBox" , parameters:  [[-10,-10,-10] , [10,10,10]] },
    sphere:   {   type: "makeSphere" , parameters: [[0,0,0] , 15 ] },
    blob:    { type: "common", parameters: ["box","sphere"]  },
    thiny:   { type: "cut",  parameters:  ["blob","cross"]  },
    root: "thiny"
    };

exports.build= function(csg_tree)
{
    "use strict";
    if (typeof csg_tree === "string") {
        csg_tree = JSON.parse(csg_tree);
    }
    var engine = new CSG_Executer();

    return engine.executeCSG(csg_tree);
};

