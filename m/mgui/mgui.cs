using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using SpiffLib;
using m;

namespace MapEdit
{
	public class Wrapper
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() {
#if NET
			// Sharp on high-DPI screens: Windows scales M's forms by its
			// font, instead of stretching a blurry bitmap of the window.
			// The forms were laid out for MS Sans Serif 8.25, .NET
			// Framework's default font, so keep it.
			Application.SetHighDpiMode(HighDpiMode.SystemAware);
			Application.SetDefaultFont(new Font("Microsoft Sans Serif", 8.25f));
#endif
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			Application.Run(new MainForm());
		}
	}
}
