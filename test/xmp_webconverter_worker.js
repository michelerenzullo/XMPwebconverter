onmessage = (e)=> {
	const folders = e.data[0];
    const f = e.data[1];
	const arguments = e.data[2];
	
    try{FS.mkdir(folders[0]);}
	catch{}
	FS.mount(WORKERFS, { files: f }, folders[0]);

    Module.options(arguments);
	
	FS.unmount(folders[0]);

	if(f.length==1){
     postMessage([
	  Object.keys(FS.open(folders[1],'r').node.contents)[0],
	  new Blob([FS.readFile(folders[1] + Object.keys(FS.open(folders[1],'r').node.contents)[0])], {type: "octet/stream"})
	 ]);
	 FS.unlink(folders[1] + Object.keys(FS.open(folders[1],'r').node.contents)[0]);
	}
	else {
	 postMessage(["converted_luts.zip",new Blob([FS.readFile("converted_luts.zip")], {type: "octet/stream"})]);
	 FS.unlink("converted_luts.zip");
	}

}

importScripts('xmp_webconverter.js');