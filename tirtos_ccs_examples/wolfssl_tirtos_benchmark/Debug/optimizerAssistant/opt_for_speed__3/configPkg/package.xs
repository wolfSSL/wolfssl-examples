/* THIS IS A GENERATED FILE -- DO NOT EDIT */

/* return the names of the generated config objects */
function getLibs(prog) {
    /* for programs, push the generated config object file into the
     * generated linker command file.
     */
    /* replace the last period in the name by an underscore */
    var name = "package/cfg/" + prog.name.replace(/\.([^.]*)$/, "_$1");
    /* base is a hack until we add cfgName to Program */
    var base = "package/cfg/" + prog.name.replace(/\.([^.]*)$/, "");
    var suffix = prog.build.target.suffix;

    var libs = [
//        name + '.o' + suffix, 
        base + '_p' + suffix + '.o' + suffix
    ];

    return libs.join(';');
}
