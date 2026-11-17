using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.Serialization;
using SpiffLib;

namespace m {
	[Serializable]
	// A wall tile. Walls are objects in the game (WallGob): each has a side
	// (neutral by default) and health, and is exported in [GameObjects]
	// (kgtWall) as well as marked in the terrain map.

	public class Wall : MapItem, ISerializable {
		private int m_nHealth;
		private Side m_side = Side.sideNeutral;
		private GobImage m_gimg;

		// Side colours (the editor's middle shade of each side)

		static private Color[] s_aclrSide = {
			Color.FromArgb(116, 116, 116),
			Color.FromArgb(0, 116, 232),
			Color.FromArgb(255, 51, 51),
			Color.FromArgb(232, 229, 0),
			Color.FromArgb(0, 255, 127),
			Color.FromArgb(220, 20, 60),
		};

		static private string[] s_astrBitmaps = {
			"Wall00",
			"Wall01",
			"Wall02",
			"Wall03",
			"Wall04",
			"Wall05",
			"Wall06",
			"Wall07",
			"Wall08",
			"Wall09",
			"Wall10",
			"Wall11",
			"Wall12",
			"Wall13",
			"Wall14",
			"Wall15",
		};

		public Wall(int nHealth) {
			m_nHealth = nHealth;
			m_gimg = Globals.GetGobImage(s_astrBitmaps[0], false);
		}

		public Wall(int nHealth, int tx, int ty) {
			m_nHealth = nHealth;
			m_tx = tx;
			m_ty = ty;
			m_gimg = Globals.GetGobImage(s_astrBitmaps[0], false);
		}

		public Wall(int nHealth, int tx, int ty, Side side) : this(nHealth, tx, ty) {
			m_side = side;
		}

		// From a [GameObjects] line: gt,side,tx,ty,wf,health

		public Wall(string strName, string strValue, int txOrigin, int tyOrigin) {
			string[] astr = strValue.Split(',');
			m_side = (Side)int.Parse(astr[1]);
			m_tx = int.Parse(astr[2]) + txOrigin;
			m_ty = int.Parse(astr[3]) + tyOrigin;
			m_nHealth = astr.Length > 5 ? int.Parse(astr[5]) : 100;
			m_gimg = Globals.GetGobImage(s_astrBitmaps[0], false);
		}

		public Wall(SerializationInfo info, StreamingContext ctx) : base(info, ctx) {
			m_nHealth = info.GetInt32("nHealth");
			try {
				m_side = (Side)info.GetInt32("Side");
			} catch (SerializationException) {
				m_side = Side.sideNeutral;
			}
			m_gimg = Globals.GetGobImage(s_astrBitmaps[0], false);
		}

		public override void GetObjectData(SerializationInfo info, StreamingContext context) {
			base.GetObjectData(info, context);
			info.AddValue("nHealth", m_nHealth);
			info.AddValue("Side", (int)m_side);
		}

		public Side Side {
			get {
				return m_side;
			}
			set {
				if (m_side != value) {
					m_side = value;
					OnPropertyChanged(this, "Side");
				}
			}
		}

		// Percent of full strength

		public int Health {
			get {
				return m_nHealth;
			}
			set {
				int n = Math.Max(1, Math.Min(100, value));
				if (m_nHealth != n) {
					m_nHealth = n;
					OnPropertyChanged(this, "Health");
				}
			}
		}

		// IMapItem

		public override Bitmap GetBitmap(Size sizTile, TemplateDoc tmpd) {
			Bitmap[] abm = m_gimg.GetBitmapSides(sizTile);
			if (m_side == Side.sideNeutral)
				return abm[0];

			// With the owner's band

			Bitmap bm = new Bitmap(abm[0]);
			using (Graphics g = Graphics.FromImage(bm))
				DrawSideBand(g, 0, 0, sizTile);
			return bm;
		}

		void DrawSideBand(Graphics g, int x, int y, Size sizTile) {
			if (m_side == Side.sideNeutral || (int)m_side >= s_aclrSide.Length)
				return;
			int cyBand = Math.Max(2, sizTile.Height / 6);
			using (SolidBrush br = new SolidBrush(s_aclrSide[(int)m_side])) {
				g.FillRectangle(br, x + sizTile.Width / 4, y + (sizTile.Height - cyBand) / 2,
						sizTile.Width / 2, cyBand);
			}
		}

		public override Point GetCenterPoint(Size sizTile) {
			Size sizGob = m_gimg.GetSize(sizTile);
			return new Point((int)m_tx * sizTile.Width + sizGob.Width / 2, (int)m_ty * sizTile.Height + sizGob.Height / 2);
		}

		public override Rectangle GetBoundingRectAt(int x, int y, Size sizTile, TemplateDoc tmpd) {
			Size sizGob = m_gimg.GetSize(sizTile);
			return new Rectangle(x, y, sizGob.Width, sizGob.Height);
		}

		public override bool HitTest(int x, int y, Size sizTile, TemplateDoc tmpd) {
			int xT = x - (int)m_tx * sizTile.Width;
			int yT = y - (int)m_ty * sizTile.Height;
			Size sizGob = m_gimg.GetSize(sizTile);
			if (xT > 0 && xT < sizGob.Width && yT > 0 && yT < sizGob.Height) {
				Bitmap[] abm = m_gimg.GetBitmapSides(sizTile);
				return abm[0].GetPixel(xT, yT) != Color.Transparent;
			}
			return false;
		}

		public override Object Clone() {
			Object[] aobj = { m_nHealth, (int)m_tx, (int)m_ty, m_side };
			return (Object)System.Activator.CreateInstance(GetType(), aobj);
		}

		public override void Draw(Graphics g, int x, int y, Size sizTile, TemplateDoc tmpd, LayerType layer, bool fSelected) {
			if (layer == LayerType.Galaxite) {
				Bitmap[] abm = m_gimg.GetBitmapSides(sizTile);
				Bitmap bm = abm[0];
				if (fSelected) {
					Rectangle rcDst = new Rectangle(x, y, bm.Width, bm.Height);
					ImageAttributes attr = new ImageAttributes();
					attr.SetGamma(0.5f);
					g.DrawImage(bm, rcDst, 0, 0, bm.Width, bm.Height, GraphicsUnit.Pixel, attr);
				} else {
					g.DrawImage(bm, x, y);
				}

				// The owner: a band of its colour across the middle

				DrawSideBand(g, x, y, sizTile);
			}
		}

		public override Ini.Property GetIniProperty(int txOrigin, int tyOrigin) {
			// [GameObjects], like a structure. For example: nil=kgtWall,kside1,38,120,0,100
			return new Ini.Property("nil", "kgtWall,k" + m_side.ToString() + "," + (m_tx - txOrigin).ToString() + "," +
					(m_ty - tyOrigin).ToString() + ",0," + m_nHealth);
		}
	}
}
