using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Resources;
using System.IO;
using System.Diagnostics;	// for Process
using SpiffLib;

namespace m
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class MainForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.MenuStrip mainMenu1;
		private System.Windows.Forms.ToolStripMenuItem menuItemFile;
		private System.Windows.Forms.ToolStripMenuItem menuItemExit;
		private System.ComponentModel.IContainer components = null;
		private System.Windows.Forms.ToolStripMenuItem menuItemOpenLevelDoc;
		private System.Windows.Forms.ToolStripMenuItem menuItemSaveLevelDoc;
		private System.Windows.Forms.ToolStripMenuItem menuItemSaveLevelDocAs;
		private System.Windows.Forms.ToolStripSeparator menuItem2;
		private System.Windows.Forms.ToolStripMenuItem menuItemNewLevelDoc;
		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.Panel panel4;
		private System.Windows.Forms.Panel panelLeft;
		private System.Windows.Forms.ToolStripMenuItem menuItem4;
		private System.Windows.Forms.ToolStripMenuItem menuItemLevelDocProperties;
		private System.Windows.Forms.ToolStripMenuItem menuItemSide1Properties;
		private System.Windows.Forms.ToolStripMenuItem menuItemSide2Properties;
		private System.Windows.Forms.ToolStripMenuItem menuItemSide3Properties;
		private System.Windows.Forms.ToolStripMenuItem menuItemSide4Properties;
		private System.Windows.Forms.ToolStripMenuItem menuItemSide5Properties;
		private System.Windows.Forms.ToolStripMenuItem menuItemSaveTileMapBitmap;
		private System.Windows.Forms.ToolStripMenuItem menuItemSaveLevelBitmap;
		private System.Windows.Forms.Splitter splitterMapRight;
		private System.Windows.Forms.Panel panelRight;
		private System.Windows.Forms.PropertyGrid propertyGrid1;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Panel panel6;
		private System.Windows.Forms.PictureBox pictureBox1;
		private System.Windows.Forms.Splitter splitterMapLeft;
		private System.Windows.Forms.ToolStripMenuItem menuItemClose;
		private System.Windows.Forms.ToolStripSeparator menuItem6;
		private System.Windows.Forms.ToolStripMenuItem menuItemWindow;
		private System.Windows.Forms.ToolStripMenuItem menuItemCascade;
		private System.Windows.Forms.ToolStripMenuItem menuItemTileHorizontal;
		private System.Windows.Forms.ToolStripMenuItem menuItemTileVertical;
		private m.GobPanel ctlGobPanel;
		private m.TemplatePanel ctlTemplatePanel;
		private System.Windows.Forms.ToolStripMenuItem menuItem1;
		private System.Windows.Forms.ToolStripMenuItem menuItemCut;
		private System.Windows.Forms.ToolStripMenuItem menuItemCopy;
		private System.Windows.Forms.ToolStripMenuItem menuItemPaste;
		private System.Windows.Forms.ToolStripMenuItem menuItemDelete;
		private System.Windows.Forms.ToolStripMenuItem menuItemRenameTemplates;
		private System.Windows.Forms.ToolStripMenuItem menuItemTest;
		private System.Windows.Forms.ToolStripMenuItem menuItemTriggers;
		private System.Windows.Forms.ToolStripMenuItem menuItemUnitGroups;
		private System.Windows.Forms.ToolStripSeparator menuItem7;
		private System.Windows.Forms.ToolStripSeparator menuItem3;
		private System.Windows.Forms.ToolStripMenuItem menuItemComments;
		private System.Windows.Forms.ToolStripSeparator menuItem9;
		private System.Windows.Forms.ToolStripMenuItem menuItemRun;
		private System.Windows.Forms.ToolStripSeparator menuItem8;
		private System.Windows.Forms.ToolStripMenuItem menuItemSwitches;
		private System.Windows.Forms.ToolStripMenuItem menuItemCounters;
		private System.Windows.Forms.ToolStripMenuItem menuItemText;
		private System.Windows.Forms.ToolStripMenuItem menuItemValidate;
		private System.Windows.Forms.ToolStripMenuItem menuItemMisc;
		private System.Windows.Forms.ToolStripSeparator menuItem10;
		private System.Windows.Forms.StatusStrip statusBar1;
		private System.Windows.Forms.ToolStripStatusLabel statusLabel1;
		private System.Windows.Forms.ToolStripMenuItem menuItem5;
		private System.Windows.Forms.ToolStripMenuItem menuItemAbout;
		private System.Windows.Forms.Splitter splitterGobTiles;
		private System.Windows.Forms.ToolStripMenuItem menuItemHelp;
		private System.Windows.Forms.ToolStripMenuItem menuItem11;
		private System.Windows.Forms.ToolStripMenuItem menuItem12;
		string m_strFileSettings = null;

		public MainForm()
		{
			new Globals();
			new DocManager();

			DocManager.SetFrameParent(this);
			DocManager.AddTemplate(new LevelDocTemplate(typeof(LevelFrame), typeof(LevelView)));
			DocManager.AddTemplate(new TemplateDocTemplate());

			// Load plug-ins. Plugins are .dll assemblies containing a class named "Plugin" 
			// that implements the IPlugin interface. Plugin dlls must be placed in the same
			// directory as the m.exe

#if STRAFER
			try {
				// UNDONE: hardwired for now. Could enumerate assemblies with a certain extension
				// for maximum extensibility convenience.

				IPlugin plug = (IPlugin)Activator.CreateInstance(@"sidewinder", "m.Plugin").Unwrap();
				Globals.Plugins.Add(plug);
			} catch {}
#endif

			// Set Kit early

			Globals.InitKit();

			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();
			Globals.PropertyGrid = propertyGrid1;
			Globals.StatusBar = statusLabel1;

			// Load settings

			LoadSettings();

			if (Globals.IsKit()) {
				// Remove stuff we don't want in the kit. Validate and Run
				// (test play in the game) stay.

				menuItemMisc.DropDownItems.Remove(menuItem8);
				menuItemMisc.DropDownItems.Remove(menuItemSaveTileMapBitmap);
				menuItemMisc.DropDownItems.Remove(menuItemSaveLevelBitmap);
				menuItemMisc.DropDownItems.Remove(menuItemRenameTemplates);
			}

			// After the main form and its menus are initialized we give plugins an opportunity
			// to modify them.

			foreach (IPlugin plug in Globals.Plugins) {
				plug.HackMenus(mainMenu1);
			}
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing ) {
			if (disposing) {
				if (components != null)
					components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent() {
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(MainForm));
			this.mainMenu1 = new System.Windows.Forms.MenuStrip();
			this.menuItemFile = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemNewLevelDoc = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemOpenLevelDoc = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemClose = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem6 = new System.Windows.Forms.ToolStripSeparator();
			this.menuItemSaveLevelDoc = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemSaveLevelDocAs = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem2 = new System.Windows.Forms.ToolStripSeparator();
			this.menuItem10 = new System.Windows.Forms.ToolStripSeparator();
			this.menuItemExit = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem1 = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemCut = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemCopy = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemPaste = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemDelete = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem4 = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemTriggers = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemUnitGroups = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemSwitches = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemCounters = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemText = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem7 = new System.Windows.Forms.ToolStripSeparator();
			this.menuItemLevelDocProperties = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem3 = new System.Windows.Forms.ToolStripSeparator();
			this.menuItemSide1Properties = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemSide2Properties = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemSide3Properties = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemSide4Properties = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemSide5Properties = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem9 = new System.Windows.Forms.ToolStripSeparator();
			this.menuItemComments = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemMisc = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemValidate = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemRun = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem8 = new System.Windows.Forms.ToolStripSeparator();
			this.menuItemSaveTileMapBitmap = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemSaveLevelBitmap = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemRenameTemplates = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemWindow = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemCascade = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemTileHorizontal = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemTileVertical = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem5 = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemHelp = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemAbout = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItemTest = new System.Windows.Forms.ToolStripMenuItem();
			this.splitterMapLeft = new System.Windows.Forms.Splitter();
			this.panelLeft = new System.Windows.Forms.Panel();
			this.panel1 = new System.Windows.Forms.Panel();
			this.panel4 = new System.Windows.Forms.Panel();
			this.ctlTemplatePanel = new m.TemplatePanel();
			this.splitterGobTiles = new System.Windows.Forms.Splitter();
			this.ctlGobPanel = new m.GobPanel();
			this.splitterMapRight = new System.Windows.Forms.Splitter();
			this.panelRight = new System.Windows.Forms.Panel();
			this.statusBar1 = new System.Windows.Forms.StatusStrip();
			this.statusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
			this.propertyGrid1 = new System.Windows.Forms.PropertyGrid();
			this.label1 = new System.Windows.Forms.Label();
			this.panel6 = new System.Windows.Forms.Panel();
			this.pictureBox1 = new System.Windows.Forms.PictureBox();
			this.menuItem11 = new System.Windows.Forms.ToolStripMenuItem();
			this.menuItem12 = new System.Windows.Forms.ToolStripMenuItem();
			this.panelLeft.SuspendLayout();
			this.panel1.SuspendLayout();
			this.panel4.SuspendLayout();
			this.panelRight.SuspendLayout();
			this.panel6.SuspendLayout();
			this.SuspendLayout();
			// 
			// mainMenu1
			// 
			this.mainMenu1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
																					  this.menuItemFile,
																					  this.menuItem1,
																					  this.menuItem4,
																					  this.menuItemMisc,
																					  this.menuItemWindow,
																					  this.menuItem5,
																					  this.menuItemTest});
			// 
			// menuItemFile
			// 
			this.menuItemFile.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
																						 this.menuItemNewLevelDoc,
																						 this.menuItemOpenLevelDoc,
																						 this.menuItemClose,
																						 this.menuItem6,
																						 this.menuItemSaveLevelDoc,
																						 this.menuItemSaveLevelDocAs,
																						 this.menuItem2,
																						 this.menuItem12,
																						 this.menuItem11,
																						 this.menuItem10,
																						 this.menuItemExit});
			this.menuItemFile.Text = "&File";
			// 
			// menuItemNewLevelDoc
			// 
			this.menuItemNewLevelDoc.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.N;
			this.menuItemNewLevelDoc.Text = "&New";
			this.menuItemNewLevelDoc.Click += new System.EventHandler(this.menuItemNewLevelDoc_Click);
			// 
			// menuItemOpenLevelDoc
			// 
			this.menuItemOpenLevelDoc.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O;
			this.menuItemOpenLevelDoc.Text = "&Open...";
			this.menuItemOpenLevelDoc.Click += new System.EventHandler(this.menuItemOpenLevelDoc_Click);
			// 
			// menuItemClose
			// 
			this.menuItemClose.Text = "&Close";
			this.menuItemClose.Click += new System.EventHandler(this.menuItemClose_Click);
			// 
			// menuItem6
			// 
			// 
			// menuItemSaveLevelDoc
			// 
			this.menuItemSaveLevelDoc.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S;
			this.menuItemSaveLevelDoc.Text = "&Save";
			this.menuItemSaveLevelDoc.Click += new System.EventHandler(this.menuItemSaveLevelDoc_Click);
			// 
			// menuItemSaveLevelDocAs
			// 
			this.menuItemSaveLevelDocAs.Text = "&Save As...";
			this.menuItemSaveLevelDocAs.Click += new System.EventHandler(this.menuItemSaveLevelDocAs_Click);
			// 
			// menuItem2
			// 
			// 
			// menuItem10
			// 
			// 
			// menuItemExit
			// 
			this.menuItemExit.Text = "&Exit";
			this.menuItemExit.Click += new System.EventHandler(this.menuItemExit_Click);
			// 
			// menuItem1
			// 
			this.menuItem1.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
																					  this.menuItemCut,
																					  this.menuItemCopy,
																					  this.menuItemPaste,
																					  this.menuItemDelete});
			this.menuItem1.Text = "&Edit";
			// 
			// menuItemCut
			// 
			this.menuItemCut.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.X;
			this.menuItemCut.Text = "Cu&t";
			this.menuItemCut.Click += new System.EventHandler(this.menuItemCut_Click);
			// 
			// menuItemCopy
			// 
			this.menuItemCopy.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C;
			this.menuItemCopy.Text = "&Copy";
			this.menuItemCopy.Click += new System.EventHandler(this.menuItemCopy_Click);
			// 
			// menuItemPaste
			// 
			this.menuItemPaste.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.P;
			this.menuItemPaste.Text = "&Paste";
			this.menuItemPaste.Click += new System.EventHandler(this.menuItemPaste_Click);
			// 
			// menuItemDelete
			// 
			this.menuItemDelete.ShortcutKeys = Keys.Delete;
			this.menuItemDelete.Text = "&Delete";
			this.menuItemDelete.Click += new System.EventHandler(this.menuItemDelete_Click);
			// 
			// menuItem4
			// 
			this.menuItem4.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
																					  this.menuItemTriggers,
																					  this.menuItemUnitGroups,
																					  this.menuItemSwitches,
																					  this.menuItemCounters,
																					  this.menuItemText,
																					  this.menuItem7,
																					  this.menuItemLevelDocProperties,
																					  this.menuItem3,
																					  this.menuItemSide1Properties,
																					  this.menuItemSide2Properties,
																					  this.menuItemSide3Properties,
																					  this.menuItemSide4Properties,
																					  this.menuItemSide5Properties,
																					  this.menuItem9,
																					  this.menuItemComments});
			this.menuItem4.Text = "&Settings";
			// 
			// menuItemTriggers
			// 
			this.menuItemTriggers.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.T;
			this.menuItemTriggers.Text = "T&riggers...";
			this.menuItemTriggers.Click += new System.EventHandler(this.menuItemTriggers_Click);
			// 
			// menuItemUnitGroups
			// 
			this.menuItemUnitGroups.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.G;
			this.menuItemUnitGroups.Text = "Unit &Groups...";
			this.menuItemUnitGroups.Click += new System.EventHandler(this.menuItemUnitGroups_Click);
			// 
			// menuItemSwitches
			// 
			this.menuItemSwitches.Text = "&Switches...";
			this.menuItemSwitches.Click += new System.EventHandler(this.menuItemSwitches_Click);
			// 
			// menuItemCounters
			// 
			this.menuItemCounters.Text = "&Counters...";
			this.menuItemCounters.Click += new System.EventHandler(this.menuItemCounters_Click);
			// 
			// menuItemText
			// 
			this.menuItemText.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.L;
			this.menuItemText.Text = "Level &Text...";
			this.menuItemText.Click += new System.EventHandler(this.menuItemText_Click);
			// 
			// menuItem7
			// 
			// 
			// menuItemLevelDocProperties
			// 
			this.menuItemLevelDocProperties.Text = "&Level";
			this.menuItemLevelDocProperties.Click += new System.EventHandler(this.menuItemLevelDocProperties_Click);
			// 
			// menuItem3
			// 
			// 
			// menuItemSide1Properties
			// 
			this.menuItemSide1Properties.Text = "Side &1 (ACME)";
			this.menuItemSide1Properties.Click += new System.EventHandler(this.menuItemSide1Properties_Click);
			// 
			// menuItemSide2Properties
			// 
			this.menuItemSide2Properties.Text = "Side &2 (OMNI)";
			this.menuItemSide2Properties.Click += new System.EventHandler(this.menuItemSide2Properties_Click);
			// 
			// menuItemSide3Properties
			// 
			this.menuItemSide3Properties.Text = "Side &3";
			this.menuItemSide3Properties.Click += new System.EventHandler(this.menuItemSide3Properties_Click);
			// 
			// menuItemSide4Properties
			// 
			this.menuItemSide4Properties.Text = "Side &4";
			this.menuItemSide4Properties.Click += new System.EventHandler(this.menuItemSide4Properties_Click);
			// 
			// menuItemSide5Properties
			// 
			this.menuItemSide5Properties.Text = "Side &5 (Crimson)";
			this.menuItemSide5Properties.Click += new System.EventHandler(this.menuItemSide5Properties_Click);
			// 
			// menuItem9
			// 
			// 
			// menuItemComments
			// 
			this.menuItemComments.ShortcutKeys = System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift | System.Windows.Forms.Keys.C;
			this.menuItemComments.Text = "Comments...";
			this.menuItemComments.Click += new System.EventHandler(this.menuItemComments_Click);
			// 
			// menuItemMisc
			// 
			this.menuItemMisc.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
																						 this.menuItemValidate,
																						 this.menuItemRun,
																						 this.menuItem8,
																						 this.menuItemSaveTileMapBitmap,
																						 this.menuItemSaveLevelBitmap,
																						 this.menuItemRenameTemplates});
			this.menuItemMisc.Text = "Misc";
			// 
			// menuItemValidate
			// 
			this.menuItemValidate.ShortcutKeys = System.Windows.Forms.Keys.F7;
			this.menuItemValidate.Text = "&Validate";
			this.menuItemValidate.Click += new System.EventHandler(this.menuItemValidate_Click);
			// 
			// menuItemRun
			// 
			this.menuItemRun.ShortcutKeys = System.Windows.Forms.Keys.F5;
			this.menuItemRun.Text = "&Run";
			this.menuItemRun.Click += new System.EventHandler(this.menuItemRun_Click);
			// 
			// menuItem8
			// 
			// 
			// menuItemSaveTileMapBitmap
			// 
			this.menuItemSaveTileMapBitmap.Text = "Save TileMap Bitmap...";
			this.menuItemSaveTileMapBitmap.Visible = false;
			this.menuItemSaveTileMapBitmap.Click += new System.EventHandler(this.menuItemSaveTileMapBitmap_Click);
			// 
			// menuItemSaveLevelBitmap
			// 
			this.menuItemSaveLevelBitmap.Text = "Save Level Bitmap...";
			this.menuItemSaveLevelBitmap.Click += new System.EventHandler(this.menuItemSaveLevelBitmap_Click);
			// 
			// menuItemRenameTemplates
			// 
			this.menuItemRenameTemplates.Text = "Rename Templates...";
			this.menuItemRenameTemplates.Click += new System.EventHandler(this.menuItemRenameTemplates_Click);
			// 
			// menuItemWindow
			// 
			this.mainMenu1.MdiWindowListItem = this.menuItemWindow;
			this.menuItemWindow.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
																						   this.menuItemCascade,
																						   this.menuItemTileHorizontal,
																						   this.menuItemTileVertical});
			this.menuItemWindow.Text = "&Window";
			// 
			// menuItemCascade
			// 
			this.menuItemCascade.Text = "&Cascade";
			this.menuItemCascade.Click += new System.EventHandler(this.menuItemCascade_Click);
			// 
			// menuItemTileHorizontal
			// 
			this.menuItemTileHorizontal.Text = "&Tile Horizontal";
			this.menuItemTileHorizontal.Click += new System.EventHandler(this.menuItemTileHorizontal_Click);
			// 
			// menuItemTileVertical
			// 
			this.menuItemTileVertical.Text = "&Tile Vertical";
			this.menuItemTileVertical.Click += new System.EventHandler(this.menuItemTileVertical_Click);
			// 
			// menuItem5
			// 
			this.menuItem5.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
																					  this.menuItemHelp,
																					  this.menuItemAbout});
			this.menuItem5.Text = "&Help";
			// 
			// menuItemHelp
			// 
			this.menuItemHelp.Text = "Help...";
			this.menuItemHelp.Click += new System.EventHandler(this.menuItemHelp_Click);
			// 
			// menuItemAbout
			// 
			this.menuItemAbout.Text = "About...";
			this.menuItemAbout.Click += new System.EventHandler(this.menuItemAbout_Click);
			// 
			// menuItemTest
			// 
			this.menuItemTest.Text = "&Test";
			this.menuItemTest.Visible = false;
			this.menuItemTest.Click += new System.EventHandler(this.menuItemTest_Click);
			// 
			// splitterMapLeft
			// 
			this.splitterMapLeft.BackColor = System.Drawing.SystemColors.Control;
			this.splitterMapLeft.Location = new System.Drawing.Point(168, 0);
			this.splitterMapLeft.Name = "splitterMapLeft";
			this.splitterMapLeft.Size = new System.Drawing.Size(3, 550);
			this.splitterMapLeft.TabIndex = 1;
			this.splitterMapLeft.TabStop = false;
			// 
			// panelLeft
			// 
			this.panelLeft.BackColor = System.Drawing.SystemColors.Control;
			this.panelLeft.Controls.Add(this.panel1);
			this.panelLeft.Dock = System.Windows.Forms.DockStyle.Left;
			this.panelLeft.ForeColor = System.Drawing.SystemColors.Control;
			this.panelLeft.Location = new System.Drawing.Point(0, 0);
			this.panelLeft.Name = "panelLeft";
			this.panelLeft.Size = new System.Drawing.Size(168, 550);
			this.panelLeft.TabIndex = 1;
			// 
			// panel1
			// 
			this.panel1.BackColor = System.Drawing.Color.Black;
			this.panel1.Controls.Add(this.panel4);
			this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel1.Location = new System.Drawing.Point(0, 0);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(168, 550);
			this.panel1.TabIndex = 1;
			// 
			// panel4
			// 
			this.panel4.BackColor = System.Drawing.Color.Black;
			this.panel4.Controls.Add(this.ctlTemplatePanel);
			this.panel4.Controls.Add(this.splitterGobTiles);
			this.panel4.Controls.Add(this.ctlGobPanel);
			this.panel4.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.panel4.Location = new System.Drawing.Point(0, 0);
			this.panel4.Name = "panel4";
			this.panel4.Size = new System.Drawing.Size(168, 550);
			this.panel4.TabIndex = 2;
			// 
			// ctlTemplatePanel
			// 
			this.ctlTemplatePanel.BackColor = System.Drawing.SystemColors.Control;
			this.ctlTemplatePanel.Dock = System.Windows.Forms.DockStyle.Fill;
			this.ctlTemplatePanel.ForeColor = System.Drawing.SystemColors.Control;
			this.ctlTemplatePanel.Location = new System.Drawing.Point(0, 219);
			this.ctlTemplatePanel.Name = "ctlTemplatePanel";
			this.ctlTemplatePanel.Size = new System.Drawing.Size(168, 331);
			this.ctlTemplatePanel.TabIndex = 3;
			this.ctlTemplatePanel.Load += new System.EventHandler(this.ctlTemplatePanel_Load);
			// 
			// splitterGobTiles
			// 
			this.splitterGobTiles.BackColor = System.Drawing.SystemColors.Control;
			this.splitterGobTiles.Dock = System.Windows.Forms.DockStyle.Top;
			this.splitterGobTiles.Location = new System.Drawing.Point(0, 216);
			this.splitterGobTiles.Name = "splitterGobTiles";
			this.splitterGobTiles.Size = new System.Drawing.Size(168, 3);
			this.splitterGobTiles.TabIndex = 2;
			this.splitterGobTiles.TabStop = false;
			// 
			// ctlGobPanel
			// 
			this.ctlGobPanel.AutoScroll = true;
			this.ctlGobPanel.BackColor = System.Drawing.Color.Black;
			this.ctlGobPanel.Dock = System.Windows.Forms.DockStyle.Top;
			this.ctlGobPanel.Location = new System.Drawing.Point(0, 0);
			this.ctlGobPanel.Name = "ctlGobPanel";
			this.ctlGobPanel.Size = new System.Drawing.Size(168, 216);
			this.ctlGobPanel.TabIndex = 0;
			// 
			// splitterMapRight
			// 
			this.splitterMapRight.BackColor = System.Drawing.SystemColors.Control;
			this.splitterMapRight.Dock = System.Windows.Forms.DockStyle.Right;
			this.splitterMapRight.Location = new System.Drawing.Point(769, 0);
			this.splitterMapRight.Name = "splitterMapRight";
			this.splitterMapRight.Size = new System.Drawing.Size(3, 550);
			this.splitterMapRight.TabIndex = 1;
			this.splitterMapRight.TabStop = false;
			// 
			// panelRight
			// 
			this.panelRight.BackColor = System.Drawing.SystemColors.Control;
			this.panelRight.Controls.Add(this.statusBar1);
			this.panelRight.Controls.Add(this.propertyGrid1);
			this.panelRight.Controls.Add(this.label1);
			this.panelRight.Controls.Add(this.panel6);
			this.panelRight.Dock = System.Windows.Forms.DockStyle.Right;
			this.panelRight.Location = new System.Drawing.Point(772, 0);
			this.panelRight.Name = "panelRight";
			this.panelRight.Size = new System.Drawing.Size(156, 550);
			this.panelRight.TabIndex = 2;
			// 
			// statusBar1
			// 
			this.statusBar1.Location = new System.Drawing.Point(0, 528);
			this.statusBar1.Name = "statusBar1";
			this.statusBar1.Size = new System.Drawing.Size(156, 22);
			this.statusBar1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
				this.statusLabel1});
			this.statusBar1.SizingGrip = false;
			this.statusBar1.TabIndex = 3;
			// 
			// statusLabel1
			// 
			this.statusLabel1.Spring = true;
			this.statusLabel1.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
			// 
			// propertyGrid1
			// 
			this.propertyGrid1.BackColor = System.Drawing.SystemColors.Control;
			this.propertyGrid1.CommandsVisibleIfAvailable = true;
			this.propertyGrid1.Cursor = System.Windows.Forms.Cursors.HSplit;
			this.propertyGrid1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.propertyGrid1.LargeButtons = false;
			this.propertyGrid1.LineColor = System.Drawing.SystemColors.ScrollBar;
			this.propertyGrid1.Location = new System.Drawing.Point(0, 123);
			this.propertyGrid1.Name = "propertyGrid1";
			this.propertyGrid1.Size = new System.Drawing.Size(156, 427);
			this.propertyGrid1.TabIndex = 2;
			this.propertyGrid1.Text = "propertyGrid1";
			this.propertyGrid1.ViewBackColor = System.Drawing.SystemColors.Window;
			this.propertyGrid1.ViewForeColor = System.Drawing.SystemColors.WindowText;
			this.propertyGrid1.SelectedObjectsChanged += new System.EventHandler(this.propertyGrid1_SelectedObjectsChanged);
			// 
			// label1
			// 
			this.label1.BackColor = System.Drawing.SystemColors.Control;
			this.label1.Dock = System.Windows.Forms.DockStyle.Top;
			this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.label1.Location = new System.Drawing.Point(0, 100);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(156, 23);
			this.label1.TabIndex = 1;
			this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// panel6
			// 
			this.panel6.BackColor = System.Drawing.SystemColors.Control;
			this.panel6.Controls.Add(this.pictureBox1);
			this.panel6.Dock = System.Windows.Forms.DockStyle.Top;
			this.panel6.Location = new System.Drawing.Point(0, 0);
			this.panel6.Name = "panel6";
			this.panel6.Size = new System.Drawing.Size(156, 100);
			this.panel6.TabIndex = 0;
			// 
			// pictureBox1
			// 
			this.pictureBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.pictureBox1.BackColor = System.Drawing.SystemColors.Control;
			this.pictureBox1.Location = new System.Drawing.Point(2, 4);
			this.pictureBox1.Name = "pictureBox1";
			this.pictureBox1.Size = new System.Drawing.Size(151, 94);
			this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
			this.pictureBox1.TabIndex = 0;
			this.pictureBox1.TabStop = false;
			// 
			// menuItem11
			// 
			this.menuItem11.Text = "Export Mission Pack...";
			this.menuItem11.Click += new System.EventHandler(this.menuItemExportMissionPack_Click);
			// 
			// menuItem12
			// 
			this.menuItem12.Text = "Import Mission Pack...";
			this.menuItem12.Click += new System.EventHandler(this.menuItemImportMissionPack_Click);
			// 
			// MainForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.BackColor = System.Drawing.SystemColors.AppWorkspace;
			this.ClientSize = new System.Drawing.Size(928, 550);
			this.Controls.Add(this.splitterMapLeft);
			this.Controls.Add(this.splitterMapRight);
			this.Controls.Add(this.panelRight);
			this.Controls.Add(this.panelLeft);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.IsMdiContainer = true;
			this.Controls.Add(this.mainMenu1);
			this.MainMenuStrip = this.mainMenu1;
			this.Name = "MainForm";
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
			this.Text = "m";
			this.Closing += new System.ComponentModel.CancelEventHandler(this.MainForm_Closing);
			this.panelLeft.ResumeLayout(false);
			this.panel1.ResumeLayout(false);
			this.panel4.ResumeLayout(false);
			this.panelRight.ResumeLayout(false);
			this.panel6.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		private void menuItemExit_Click(object sender, System.EventArgs e)
		{
			this.Close();
		}

		private void menuItemNewLevelDoc_Click(object sender, System.EventArgs e) {
			DocManager.NewDocument(typeof(LevelDoc), null);
		}

		private void menuItemOpenLevelDoc_Click(object sender, System.EventArgs e) {
			// Allow both .ld and .tc opening

			OpenFileDialog frmOpen = new OpenFileDialog();
			frmOpen.DefaultExt = "ld";
			frmOpen.Filter = "M files (*.ld;*.tc)|*.ld;*.tc";
			frmOpen.Title = "Open...";
			if (frmOpen.ShowDialog() != DialogResult.Cancel)
				DocManager.OpenDocument(frmOpen.FileName);
		}

		private void menuItemSaveLevelDoc_Click(object sender, System.EventArgs e) {
			Document doc = DocManager.GetActiveDocument(typeof(LevelDoc));
			if (doc != null)
				doc.Save();
		}

		private void menuItemSaveLevelDocAs_Click(object sender, System.EventArgs e) {
			Document doc = DocManager.GetActiveDocument(typeof(LevelDoc));
			if (doc != null)
				doc.SaveAs(null);
		}

		private void menuItemSaveTileMapBitmap_Click(object sender, System.EventArgs e) {
#if false
			SaveFileDialog frmSave = new SaveFileDialog();
			frmSave.DefaultExt = "png";
			frmSave.Filter = "Png Files (*.png)|*.png";
			frmSave.Title = "Save TileMap Bitmap As";
			if (frmSave.ShowDialog() == DialogResult.Cancel)
				return;
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument();
			if (lvld != null)
				lvld.GetMapBitmap().Save(frmSave.FileName, ImageFormat.Png);
#endif
		}

		private void menuItemSaveLevelBitmap_Click(object sender, System.EventArgs e) {
			SaveFileDialog frmSave = new SaveFileDialog();
			frmSave.DefaultExt = "png";
			frmSave.Filter = "Png Files (*.png)|*.png";
			frmSave.Title = "Save Level Bitmap As";
			if (frmSave.ShowDialog() == DialogResult.Cancel)
				return;
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null) {
				TemplateDoc tmpd = lvld.GetTemplateDoc();
				lvld.GetMapBitmap(tmpd.TileSize, tmpd, false).Save(frmSave.FileName, ImageFormat.Png);
			}
		}

		private void menuItemLevelDocProperties_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				Globals.PropertyGrid.SelectedObject = lvld;
		}

		private void propertyGrid1_SelectedObjectsChanged(object sender, System.EventArgs e) {
			TemplateDoc tmpd = (TemplateDoc)DocManager.GetActiveDocument(typeof(TemplateDoc));
			Object obj = Globals.PropertyGrid.SelectedObject;
			if (obj == null) {
				label1.Text = null;
				pictureBox1.Image = null;
			} else {
				string strName = obj.GetType().Name;
				if (obj is Unit && !(obj is Activator))
					strName = Helper.GetDisplayName(typeof(UnitType), "kut" + strName);
				else if (obj is Tile)
					strName = ((Tile)obj).Name;
				label1.Text = strName;
				IMapItem mi = obj as IMapItem;
				if (mi == null) {
					pictureBox1.Image = null;
				} else {
					Tile tile = mi as Tile;
					if (tile != null) {
						pictureBox1.Image = Misc.TraceEdges(tile.GetBitmap(new Size(tmpd.TileSize.Width, tmpd.TileSize.Height), tmpd), 1, Color.Black);
					} else {
						pictureBox1.Image = mi.GetBitmap(new Size(tmpd.TileSize.Width, tmpd.TileSize.Height), tmpd);
					}
				}
			}
		}

		private void menuItemSide1Properties_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				Globals.PropertyGrid.SelectedObject = lvld.GetSideInfo(Side.side1);
		}

		private void menuItemSide2Properties_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				Globals.PropertyGrid.SelectedObject = lvld.GetSideInfo(Side.side2);
		}

		private void menuItemSide3Properties_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				Globals.PropertyGrid.SelectedObject = lvld.GetSideInfo(Side.side3);
		}

		private void menuItemSide4Properties_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				Globals.PropertyGrid.SelectedObject = lvld.GetSideInfo(Side.side4);
		}

		private void menuItemSide5Properties_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				Globals.PropertyGrid.SelectedObject = lvld.GetSideInfo(Side.side5);
		}

		private void MainForm_Closing(object sender, System.ComponentModel.CancelEventArgs e) {
			if (!DocManager.CloseAllDocuments()) {
				e.Cancel = true;
				return;
			}

			// Modifiers to toggle kit mode

			if ((Control.ModifierKeys & Keys.Shift) == Keys.Shift) {
				Globals.SetKit(!Globals.IsKit());
			}

			SaveSettings();
		}

		void SaveSettings() {
			// Save settings in ini.
			
			Ini ini = new Ini();			
			Ini.Section secGeneral = new Ini.Section("General");
			secGeneral.Add(new Ini.Property("WindowState", WindowState.ToString()));
			// The normal (not maximized) position, which also says which
			// monitor M is on
			Rectangle rcNormal = WindowState == FormWindowState.Normal ? Bounds : RestoreBounds;
			secGeneral.Add(new Ini.Property("X", rcNormal.X.ToString()));
			secGeneral.Add(new Ini.Property("Y", rcNormal.Y.ToString()));
			secGeneral.Add(new Ini.Property("Width", rcNormal.Width.ToString()));
			secGeneral.Add(new Ini.Property("Height", rcNormal.Height.ToString()));
			if (GamePath != null)
				secGeneral.Add(new Ini.Property("GamePath", GamePath));
			secGeneral.Add(new Ini.Property("Kit", Globals.IsKit().ToString()));
			secGeneral.Add(new Ini.Property("Eula", "1"));

			ini.Add(secGeneral);
			
			// Place in directory where .exe resides
			
			try {
				if (m_strFileSettings != null)
					ini.Save(m_strFileSettings);
			} catch {
			}
		}

		void LoadSettings() {
			// Settings

			m_strFileSettings = Application.ExecutablePath.Replace(".exe", ".ini");

			Ini ini;
			try {
				ini = new Ini(m_strFileSettings);
			} catch {
				ini = null;
			}

			if (ini == null)
				return;

			Ini.Section sec = ini["General"];
			if (sec == null)
				return;
			Ini.Property prop = sec["WindowState"];
			if (prop != null) {
				// Restore the normal position first, so a maximized M
				// maximizes on the monitor it was on. Skip it if that
				// monitor is gone.

				try {
					Rectangle rc = new Rectangle();
					rc.X = int.Parse(sec["X"].Value);
					rc.Y = int.Parse(sec["Y"].Value);
					rc.Width = int.Parse(sec["Width"].Value);
					rc.Height = int.Parse(sec["Height"].Value);
					bool fOnScreen = false;
					foreach (Screen scr in Screen.AllScreens) {
						if (scr.WorkingArea.IntersectsWith(rc))
							fOnScreen = true;
					}
					if (fOnScreen && rc.Width > 0 && rc.Height > 0) {
						StartPosition = FormStartPosition.Manual;
						Bounds = rc;
					}
				} catch (Exception) {
				}

				switch(prop.Value) {
				case "Maximized":
					WindowState = FormWindowState.Maximized;
					break;

				case "Normal":
					WindowState = FormWindowState.Normal;
					break;
				}
			}

			prop = sec["GamePath"];
			if (prop != null && prop.Value.Length != 0)
				GamePath = prop.Value;
		}

		// Path of HostileTakeover.exe, used by Run (F5). Saved in m.ini.

		private string m_strGamePath;

		public string GamePath {
			get {
				return m_strGamePath;
			}
			set {
				m_strGamePath = value;
			}
		}

		const string kstrGameExe = "HostileTakeover.exe";

		// Finds the game: the GamePath setting, next to M, or the CMake build
		// folders of this source tree. Asks if it can't be found.

		string FindGame() {
			if (GamePath != null && File.Exists(GamePath))
				return GamePath;

			string strDir = Path.GetDirectoryName(Application.ExecutablePath);
			string[] astrCandidates = {
				Path.Combine(strDir, kstrGameExe),
				Path.Combine(strDir, Path.Combine("..", kstrGameExe)),
				Path.Combine(strDir, Path.Combine("..", Path.Combine("build", Path.Combine("bin", Path.Combine("Release", kstrGameExe))))),
				Path.Combine(strDir, Path.Combine("..", Path.Combine("build", Path.Combine("bin", Path.Combine("RelWithDebInfo", kstrGameExe))))),
				Path.Combine(strDir, Path.Combine("..", Path.Combine("build", Path.Combine("bin", Path.Combine("Debug", kstrGameExe))))),
			};
			foreach (string strPath in astrCandidates) {
				if (File.Exists(strPath))
					return Path.GetFullPath(strPath);
			}

			OpenFileDialog frmOpen = new OpenFileDialog();
			frmOpen.Title = "Where is Hostile Takeover?";
			frmOpen.Filter = "Hostile Takeover (" + kstrGameExe + ")|" + kstrGameExe;
			if (frmOpen.ShowDialog() == DialogResult.Cancel)
				return null;
			GamePath = frmOpen.FileName;
			return GamePath;
		}

		private void menuItemCascade_Click(object sender, System.EventArgs e) {
			LayoutMdi(MdiLayout.Cascade);
		}

		private void menuItemTileHorizontal_Click(object sender, System.EventArgs e) {
			LayoutMdi(MdiLayout.TileHorizontal);
		}

		private void menuItemTileVertical_Click(object sender, System.EventArgs e) {
			LayoutMdi(MdiLayout.TileVertical);
		}

		private void menuItemCut_Click(object sender, System.EventArgs e) {
			ICommandTarget cmdt = DocManager.GetCommandTarget();
			if (cmdt != null)
				cmdt.DispatchCommand(Command.Cut);
		}

		private void menuItemCopy_Click(object sender, System.EventArgs e) {
			ICommandTarget cmdt = DocManager.GetCommandTarget();
			if (cmdt != null)
				cmdt.DispatchCommand(Command.Copy);
		}

		private void menuItemPaste_Click(object sender, System.EventArgs e) {
			ICommandTarget cmdt = DocManager.GetCommandTarget();
			if (cmdt != null)
				cmdt.DispatchCommand(Command.Paste);
		}

		private void menuItemDelete_Click(object sender, System.EventArgs e) {
			ICommandTarget cmdt = DocManager.GetCommandTarget();
			if (cmdt != null)
				cmdt.DispatchCommand(Command.Delete);
		}

		private void menuItemClose_Click(object sender, System.EventArgs e) {
			if (ActiveMdiChild != null)
				ActiveMdiChild.Close();
		}

		private void menuItemRenameTemplates_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld == null)
				return;

			OpenFileDialog ofd = new OpenFileDialog();

			ofd.Filter = "txt files (*.txt)|*.txt|All files (*.*)|*.*" ;

			if (ofd.ShowDialog() != DialogResult.OK)
				return;

			StreamReader stmrTemplateNames = new StreamReader(ofd.FileName);

			TemplateDoc tmpd = lvld.GetTemplateDoc();
			Template[] atmpl = tmpd.GetTemplates();
			foreach (Template tmpl in atmpl) {
				string strT = stmrTemplateNames.ReadLine();
				if (strT == null)
					break;
				tmpl.Name = strT;
			}

			stmrTemplateNames.Close();
		}

		private void menuItemTest_Click(object sender, System.EventArgs e) {
			TriggersForm frm = new TriggersForm(new TriggerManager());
			frm.ShowDialog();
		}

		private void menuItemTriggers_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				lvld.EditTriggers();
		}

		private void menuItemUnitGroups_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				lvld.EditUnitGroups();
		}

		private void menuItemComments_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				lvld.EditComments();
		}

		private void menuItemSwitches_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				SwitchesForm.DoModal("Switch", "", CaTypeSwitch.GetSwitchNames());
		}

		private void menuItemCounters_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld != null)
				CountersForm.DoModal("Counter", "", CaTypeCounter.GetCounterNames());
		}

		// Run (F5): plays the active level in the game. The level is exported
		// to a temporary mission pack and the game is started with
		// -pdb <pack> -l <level>, which goes straight into the level.

		private void menuItemRun_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld == null)
				return;
			lvld.Save();

			// Check it first; the game may not cope with an invalid level

			OutputForm.HideIt();
			OutputForm.Clear();
			int cErrors = lvld.Validate(new LevelDoc.ValidateErrorDelegate(ValidateCallback));
			if (cErrors != 0) {
				if (MessageBox.Show(DocManager.GetFrameParent(), lvld.GetName() + " has " + cErrors +
						" error(s), listed in the Output window. Run it anyway?", "Run",
						MessageBoxButtons.YesNo) != DialogResult.Yes)
					return;
			}

			string strGame = FindGame();
			if (strGame == null)
				return;

			// Only one copy of the game can run at a time

			while (Process.GetProcessesByName(Path.GetFileNameWithoutExtension(strGame)).Length != 0) {
				if (MessageBox.Show(DocManager.GetFrameParent(), "Please close Hostile Takeover, then click Retry.", "Hostile Takeover is running", MessageBoxButtons.RetryCancel) == DialogResult.Cancel)
					return;
			}

			string strPack = Path.Combine(Path.GetTempPath(), "HostileTakeoverRun.pdb");
			try {
				OutputTools.SaveExpansionPdb(strPack, new Document[] { lvld }, "1.1");
			} catch (Exception ex) {
				MessageBox.Show(DocManager.GetFrameParent(), "Couldn't export the level: " + ex.Message, "Run");
				return;
			}

			ProcessStartInfo psi = new ProcessStartInfo(strGame);
			psi.Arguments = "-pdb \"" + strPack + "\" -l \"" + OutputTools.GetExpansionLevelName(lvld) + "\"";
			psi.WorkingDirectory = Path.GetDirectoryName(strGame);
			psi.UseShellExecute = false;
			try {
				Process.Start(psi);
			} catch (Win32Exception ex) {
				MessageBox.Show(DocManager.GetFrameParent(), "Couldn't start " + strGame + ": " + ex.Message, "Run");
				GamePath = null;
			}
		}

		private void menuItemText_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld == null)
				return;
			lvld.EditLevelText();
		}

		private void menuItemValidate_Click(object sender, System.EventArgs e) {
			LevelDoc lvld = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));
			if (lvld == null)
				return;
			OutputForm.HideIt();
			OutputForm.Clear();
			lvld.Validate(new LevelDoc.ValidateErrorDelegate(ValidateCallback));
		}

		//c:\code\ht\m\outputform.cs(21,15): error CS0111:
		private void ValidateCallback(LevelDoc lvld, LevelDoc.ValidateError ve, int tx, int ty, object ob, string str) {
			OutputForm.ShowIt();
#if false
			OutputForm.Error(lvld, ob, "({0},{1},{2}): {3}: {4}\n", tx, ty, ob == null ? "" : ob.GetHashCode().ToString(), ve.ToString(), str);
#else
			if (ob is Unit) {
				OutputForm.Error(lvld, ob, "({0},{1}): {2}: Side {3} {4}\n", tx, ty, ve.ToString(), ((Unit)ob).Side, str);
			} else {
				OutputForm.Error(lvld, ob, "({0},{1}): {2}: {3}\n", tx, ty, ve.ToString(), str);
			}
#endif
		}

		private void ctlTemplatePanel_Load(object sender, System.EventArgs e) {
		
		}

		private void menuItemImportMissionPack_Click(object sender, System.EventArgs e) {
			// Prompt the user to save and close existing missions. This is so if a user
			// imports more than once, missions aren't accumulated by mistake. 

#if false // annoying
			LevelDocTemplate doctLevel = (LevelDocTemplate)DocManager.FindDocTemplate(typeof(LevelDoc));
			Document[] adoc = doctLevel.GetDocuments();
			if (adoc.Length != 0) {
				MessageBox.Show(DocManager.GetFrameParent(), "Close all Level Documents before importing");
				return;
			}
#endif

			// Select the pdb to load, and import it

			OpenFileDialog frmOpen = new OpenFileDialog();
			frmOpen.DefaultExt = "pdb";
			frmOpen.Filter = "Pdb files (*.pdb)|*.pdb";
			frmOpen.Title = "Open Mission Pack...";
			if (frmOpen.ShowDialog() != DialogResult.Cancel)
				OutputTools.ImportExpansionPdb(frmOpen.FileName);
		}

		private void menuItemExportMissionPack_Click(object sender, System.EventArgs e) {
			// Any leveldocs to save?

			LevelDocTemplate doctLevel = (LevelDocTemplate)DocManager.FindDocTemplate(typeof(LevelDoc));
			Document[] adoc = doctLevel.GetDocuments();
			if (adoc.Length == 0) {
				MessageBox.Show(DocManager.GetFrameParent(), "No Level Descriptions loaded!");
				return;
			}

			// First save all level docs

			//annoying
			//DocManager.SaveAllModified(typeof(LevelDoc));

			// Remember this

			LevelDoc lvldActive = (LevelDoc)DocManager.GetActiveDocument(typeof(LevelDoc));

			// Validate these first

			OutputForm.HideIt();
			OutputForm.Clear();
			foreach (LevelDoc lvld in adoc) {
				// Unfortunately it seems we have a number of methods during validation and SaveIni-ing that
				// rely on which document is active.
				DocManager.SetActiveDocument(typeof(LevelDoc), lvld);
				int cErrors = lvld.Validate(new LevelDoc.ValidateErrorDelegate(ValidateCallback));
				if (cErrors != 0) {
					MessageBox.Show(DocManager.GetFrameParent(), "Please fix errors in " + lvld.GetName() + " then try again.");
					return;
				}
			}

			// Restore

			if (lvldActive != null)
				DocManager.SetActiveDocument(typeof(LevelDoc), lvldActive);

			// First get the filename

			SaveFileDialog frmSave = new SaveFileDialog();
			frmSave.DefaultExt = "pdb";
			frmSave.Filter = "Mission Pack Files (*.pdb)|*.pdb";
			frmSave.Title = "Export Mission Pack";
			if (frmSave.ShowDialog() == DialogResult.Cancel)
				return;

			// Export expansion .pdb

			OutputTools.SaveExpansionPdb(frmSave.FileName, adoc, "1.1");
			MessageBox.Show(DocManager.GetFrameParent(), "Saved " + Path.GetFileName(frmSave.FileName) +
					".\n\nTo play it, copy it into the Missions folder next to HostileTakeover.exe, " +
					"or into %APPDATA%\\Spiffcode\\HostileTakeover\\Missions. " +
					"Its missions appear under Play > Custom.", "Export Mission Pack");
		}

		private void menuItemAbout_Click(object sender, System.EventArgs e) {
			AboutForm frm = new AboutForm();
			frm.ShowDialog();
		}

		private void menuItemHelp_Click(object sender, System.EventArgs e) {
			if (m_strFileSettings.EndsWith("mgui.ini")) {
				System.Diagnostics.Process.Start(m_strFileSettings.Replace("mgui.ini", "m.chm"));
			} else if (m_strFileSettings.EndsWith("m.ini")) {
				System.Diagnostics.Process.Start(m_strFileSettings.Replace("m.ini", "m.chm"));
			}
		}
	}

	//
	// Plug-in interface
	//

	public interface IPlugin {
		void HackMenus(MenuStrip mnu);
		IMapItem[] GetMapItems();
	}
}
