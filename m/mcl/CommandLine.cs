using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Collections;
using System.IO;
using SpiffLib;
using System.Diagnostics;
using m;

namespace m
{
	class CommandLine
	{
		public static void Main(string[] astr)
		{
			new Globals();
			new DocManager();

			DocManager.AddTemplate(new LevelDocTemplate(null, null));
			DocManager.AddTemplate(new TemplateDocTemplate());

			switch (astr[0]) {
			case "-mixmaps":
				OutputTools.ExportMixMaps(astr);
				break;

			case "-levels":
				OutputTools.ExportLevels(astr, 0);
				break;

			case "-images":
				OutputTools.ExportImages(astr);
				break;

			case "-makepal":
				OutputTools.MakePalette(astr);
				break;

			case "-special":
				char sep = Path.DirectorySeparatorChar;
				string root = sep + "ht" + sep + "data" + sep;
				OutputTools.MixMapImportSpecial(Theater.Desert, (TemplateDoc)DocManager.OpenDocument(root + "desert.tc"), root + "desert24.tc");
				OutputTools.MixMapImportSpecial(Theater.Temperate, (TemplateDoc)DocManager.OpenDocument(root + "temperate.tc"), root + "temperate24.tc");
				break;

			case "-exporttext":
				OutputTools.ExportText(astr);
				break;

			case "-importtext":
				OutputTools.ImportText(astr);
				break;

			case "-testimport":
				OutputTools.ImportExportPdbs(astr);
				break;

			case "-pack":
				// -pack out.pdb level.ld [level.ld ...]
				// Exports levels to a mission pack, like M's Export Mission Pack

				{
					ArrayList alsDocs = new ArrayList();
					for (int i = 2; i < astr.Length; i++) {
						Document doc = DocManager.OpenDocument(astr[i]);
						if (doc == null) {
							Console.WriteLine("Error: couldn't open " + astr[i]);
							Environment.Exit(1);
						}
						alsDocs.Add(doc);
					}
					OutputTools.SaveExpansionPdb(astr[1], (Document[])alsDocs.ToArray(typeof(Document)), "1.1");
					Console.WriteLine("Wrote " + astr[1]);
				}
				break;
			}
		}
	}
}
