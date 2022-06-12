onmessage = (e) => {
	const files = e.data[0];
	const args = e.data[1];
	args["input"] = "to_convert/"
	args["output"] = "converted/"

	/* if (!Object.keys(FS.open('/','r').node.contents).includes(args.input.endsWith("/") ? args.input.slice(0,-1) : args.input))
		FS.mkdir(args.input);*/

	try { FS.mkdir(args.input); }
	catch { }

	FS.mount(WORKERFS, { files: files }, args.input);

	Module.options(args);

	FS.unmount(args.input);

	try {
		const filename = Object.keys(FS.open(args.output, 'r').node.contents)[0];
		if (filename) {
			postMessage([
				filename,
				new Blob([FS.readFile(args.output + filename)], { type: "octet/stream" })
			]);
			FS.unlink(args.output + filename);
		}
		else {
			postMessage(["converted_luts.zip", new Blob([FS.readFile("converted_luts.zip")], { type: "octet/stream" })]);
			FS.unlink("converted_luts.zip");
		}
	}
	catch { postMessage(undefined); }

}

importScripts('xmp_webconverter.js');