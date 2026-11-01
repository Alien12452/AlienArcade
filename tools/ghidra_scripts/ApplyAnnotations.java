// Apply names from a TSV produced by tools/ghidra_sync.py to the current program.
//
// Each non-empty line is KIND<TAB>ADDRESS<TAB>NAME:
//   KIND     FUNCTION, STUB, GLOBAL, or LIBRARY (GLOBAL creates a label;
//            anything else creates/renames a function)
//   ADDRESS  Ghidra segmented address, e.g. 1008:0000 for NE 2:0000
//   NAME     symbol to apply
//
// Invoked headless by ghidra_sync.py:
//   analyzeHeadless ... -postScript ApplyAnnotations.java build/ghidra_annotations.tsv
//
// @category AlienArcade

import java.nio.file.Files;
import java.nio.file.Path;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class ApplyAnnotations extends GhidraScript {
	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length != 1) {
			printerr("usage: ApplyAnnotations.java <annotations.tsv>");
			return;
		}
		int applied = 0;
		for (String line : Files.readAllLines(Path.of(args[0]))) {
			String[] f = line.split("\t");
			if (f.length != 3) {
				continue;
			}
			Address addr = toAddr(f[1]);
			if (f[0].equals("GLOBAL")) {
				createLabel(addr, f[2], true, SourceType.USER_DEFINED);
			}
			else {
				Function func = getFunctionAt(addr);
				if (func == null) {
					func = createFunction(addr, f[2]);
				}
				if (func == null) {
					printerr("could not create function at " + addr);
					continue;
				}
				func.setName(f[2], SourceType.USER_DEFINED);
			}
			applied++;
		}
		println("applied " + applied + " annotations");
	}
}
