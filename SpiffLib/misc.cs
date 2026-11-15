using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Collections;

namespace SpiffLib
{
	/// <summary>
	/// The Misc class encapsulates various static helper methods that lack a more
	/// appropriate home.
	/// </summary>
	public class Misc
	{
		class Preprocessor {
			Hashtable m_htbl = new Hashtable();

			public Preprocessor(Stream stmInclude) {
				// Parse stmInclude and make a hash table

				Regex rexI = new Regex(@"^\#define\s*(?<name>\w+)\s*(?<value>[\-0-9]+).*$");

				stmInclude.Seek(0, SeekOrigin.Begin);
				TextReader tr = new StreamReader(stmInclude);
				while (true) {
					string strT = tr.ReadLine();
					if (strT == null)
						break;

					// Remove white space

					strT.Trim();
					if (strT.Length == 0)
						continue;

					// Comment?

					if (strT.StartsWith("//"))
						continue;

					// Match against regex

					Match mat = rexI.Match(strT);
					string strName = mat.Groups["name"].Value;
					int nValue = Int32.Parse(mat.Groups["value"].Value);
					if (strName.Length != 0)
						m_htbl.Add(strName, nValue);
				}
			}

			public MemoryStream Preprocess(Stream stmContent) {
				// Go through stmContent and replace matches with values

				stmContent.Seek(0, SeekOrigin.Begin);
				TextReader tr = new StreamReader(stmContent);
				MemoryStream stm = new MemoryStream();
				TextWriter tw = new StreamWriter(stm);

				while (true) {
					string strT = tr.ReadLine();
					if (strT == null)
						break;

					// Parse for alpha numeric sequences

					string strResult = Regex.Replace(strT, @"\w+", new MatchEvaluator(ReplaceText));
					tw.WriteLine(strResult);
				}

				tw.Flush();
				return stm;
			}

			string ReplaceText(Match mat) {
				string str = mat.ToString();
				if (!m_htbl.ContainsKey(str))
					return str;
				return ((int)m_htbl[str]).ToString();
			}
		}
		
		public static MemoryStream PreprocessStream(Stream stmContent, Stream stmInclude) {
			Preprocessor pp = new Preprocessor(stmInclude);
			return pp.Preprocess(stmContent);
		}

		public static byte[] GetByteArrayFromString(string str) {
			byte[] ab = new byte[str.Length + 1];
			for (int n = 0; n < str.Length; n++)
				ab[n] = (byte)str[n];
			ab[str.Length] = 0;
			return ab;
		}

		/// <summary>
		/// Takes RGB values as floats in the range from 0 to 1 and returns
		/// HSL values also in the range from 0 to 1.
		///
		/// Thank you Ken Fishkin of Pixar Inc and Graphics Gems
		/// </summary>
		/// <param name="r"></param>
		/// <param name="g"></param>
		/// <param name="b"></param>
		/// <param name="h"></param>
		/// <param name="s"></param>
		/// <param name="l"></param>
		public static unsafe void RgbToHsl(float r, float g, float b, float *h, float *s, float *l) {
			float v;
			float m;
			float vm;
			float r2, g2, b2;

			v = Math.Max(r,g);
			v = Math.Max(v,b);
			m = Math.Min(r,g);
			m = Math.Min(m,b);

			*h = 0;
			*s = 0;
			if ((*l = (m + v) / 2.0f) <= 0.0f) 
				return;
			if ((*s = vm = v - m) > 0.0f) {
				*s /= (*l <= 0.5f) ? (v + m ) : (2.0f - v - m) ;
			} else
				return;

			r2 = (v - r) / vm;
			g2 = (v - g) / vm;
			b2 = (v - b) / vm;

			if (r == v)
				*h = (g == m ? 5.0f + b2 : 1.0f - g2);
			else if (g == v)
				*h = (b == m ? 1.0f + r2 : 3.0f - b2);
			else
				*h = (r == m ? 3.0f + g2 : 5.0f - r2);

			*h /= 6;
		}

		/// <summary>
		/// Takes HSL values as floats in the range from 0 to 1 and returns
		/// RGB values also in the range from 0 to 1.
		///
		/// Thank you Ken Fishkin of Pixar Inc and Graphics Gems
		/// </summary>
		/// <param name="h"></param>
		/// <param name="s"></param>
		/// <param name="l"></param>
		/// <param name="r"></param>
		/// <param name="g"></param>
		/// <param name="b"></param>
		public static unsafe void HslToRgb(float h, float s, float l, float *r, float *g, float *b) {
			float v;

			v = (l <= 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
			if (v <= 0) {
				*r = *g = *b = 0.0f;
			} else {
				float m;
				float sv;
				int sextant;
				float fract, vsf, mid1, mid2;

				m = l + l - v;
				sv = (v - m ) / v;
				h *= 6.0f;
				sextant = (int)h;	
				fract = h - sextant;
				vsf = v * sv * fract;
				mid1 = m + vsf;
				mid2 = v - vsf;
				switch (sextant) {
				case 0: *r = v; *g = mid1; *b = m; break;
				case 1: *r = mid2; *g = v; *b = m; break;
				case 2: *r = m; *g = v; *b = mid1; break;
				case 3: *r = m; *g = mid2; *b = v; break;
				case 4: *r = mid1; *g = m; *b = v; break;
				case 5: *r = v; *g = m; *b = mid2; break;
				}
			}
		}

		/// <summary>
		/// All this routine does is create a duplicate of the passed-in bitmap. Its utility
		/// comes from the fact that it doesn't duplicate the properties (e.g., horizontal 
		/// and vertical resolution) that are attached to bitmaps after they've been read
		/// from files. These properties can interfere with the expected operation of various
		/// Bitmap/Graphics methods (e.g., DragImage will inexplicably scale the bitmap when
		/// drawn from based on its intrinsic resolution properties). We would prefer to just
		/// clear out these properties or force them to be ignored but all such attempts to
		/// date have been rebuffed.
		/// </summary>
		/// <param name="bmSrc">The Bitmap to be 'normalized'</param>
		/// <returns>A property-free Bitmap</returns>
		public static unsafe Bitmap NormalizeBitmap(Bitmap bmSrc) {
			Bitmap bmDst = new Bitmap(bmSrc.Width, bmSrc.Height, bmSrc.PixelFormat);

			Rectangle rc = new Rectangle(0, 0, bmSrc.Width, bmSrc.Height);
			BitmapData bmdSrc = bmSrc.LockBits(rc, ImageLockMode.ReadOnly, bmSrc.PixelFormat);
			BitmapData bmdDst = bmDst.LockBits(rc, ImageLockMode.WriteOnly, bmSrc.PixelFormat);

			int cul = bmdSrc.Height * bmdSrc.Stride / sizeof(ulong);
			ulong *pulSrc = (ulong *)bmdSrc.Scan0;
			ulong *pulDst = (ulong *)bmdDst.Scan0;
			while (cul-- > 0) {
				*pulDst++ = *pulSrc++;
			}
			
			bmSrc.UnlockBits(bmdSrc);
			bmDst.UnlockBits(bmdDst);
			
			return bmDst;
		}

		static Color s_clrShadow = Color.FromArgb(156, 212, 248);
		static Color s_clrTransparent = Color.FromArgb(255, 0, 255);

		/// <summary>
		/// Scan from the bottom up. First scanline containing non-shadow, non-transparent color
		/// is the baseline.
		/// </summary>
		/// <param name="bm"></param>
		/// <returns>A scanline offset from the top of the image</returns>
		public static unsafe int FindBaseline(Bitmap bm) {
			Rectangle rc = new Rectangle(0, 0, bm.Width, bm.Height);
			BitmapData bmd = bm.LockBits(rc, ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
			
			byte *pbBase = (byte *)bmd.Scan0;
			for (int y = bm.Height - 1; y >= 0; y--) {
				byte *pb = pbBase + y * bmd.Stride;
				for (int x = 0; x < bm.Width; x++, pb += 3) {
					Color clr = Color.FromArgb(pb[2], pb[1], pb[0]);
					if (clr != s_clrTransparent && clr != s_clrShadow) {
						bm.UnlockBits(bmd);
						return y;
					}
				}
			}
			
			bm.UnlockBits(bmd);
			return 0;
		}

		/// <summary>
		/// The SwapUShort method swaps the high and low bytes of the passed ushort
		/// value. It's good for converting between big and little-endian ushort values.
		/// </summary>
		/// <param name="us">The ushort value whose bytes to swap</param>
		/// <returns>The byte-swapped ushort</returns>
		public static ushort SwapUShort(ushort us) {
			int n = (int)us;
			return (ushort)(((n >> 8) & 0x00ff) | ((n << 8) & 0xff00));
		}

		/// <summary>
		/// The SwapUInt method swaps the endian order of a 4 byte type.
		/// </summary>
		/// <param name="ui">The uint value whose bytes to swap</param>
		/// <returns>The byte-swapped uint</returns>
		public static uint SwapUInt(uint ui) {
			return ((((ui)&0xFF)<<24) | (((ui)&0xFF00)<<8) | (((ui)&0xFF0000)>>8) | (((ui)&0xFF000000)>>24));
		}

		/// <summary>
		/// This cool method returns a new bitmap which has a copy of the passed-in bitmap
		/// surrounded by a colored outline.
		/// </summary>
		/// <param name="bm">Bitmap to outline</param>
		/// <param name="cp">width/height of outline in pixels</param>
		/// <param name="clrEdge">color of the outline</param>
		/// <returns>Bitmap with traced edges</returns>
		public static Bitmap TraceEdges(Bitmap bm, int cp, Color clrEdge) {
			// Create a mask of sorts
			Color clrTransparent = Color.FromArgb(255, 0, 255);
			Bitmap bmEdge = new Bitmap(bm.Width, bm.Height);
			Graphics gEdge = Graphics.FromImage(bmEdge);
			gEdge.Clear(clrTransparent);
			gEdge.DrawImage(bm, 0, 0);
			for (int x = 0; x < bmEdge.Width; x++) {
				for (int y = 0; y < bmEdge.Height; y++) {
					Color clrT = bmEdge.GetPixel(x, y);
					if (clrT != clrTransparent) {
						bmEdge.SetPixel(x, y, clrEdge);
					}
				}
			}
			bmEdge.MakeTransparent(clrTransparent);

			// Draw the mask aligned to create edges
			Bitmap bmT = new Bitmap(bm.Width + cp * 2, bm.Height + cp * 2);
			Graphics gT = Graphics.FromImage(bmT);
			gT.Clear(clrTransparent);
			gT.DrawImage(bmEdge, 0, 0);
			gT.DrawImage(bmEdge, cp, 0);
			gT.DrawImage(bmEdge, cp * 2, 0);
			gT.DrawImage(bmEdge, cp * 2, cp);
			gT.DrawImage(bmEdge, cp * 2, cp * 2);
			gT.DrawImage(bmEdge, cp, cp * 2);
			gT.DrawImage(bmEdge, 0, cp * 2);
			gT.DrawImage(bmEdge, 0, cp);

			// Draw the actual image in the middle; we're done
			gT.DrawImage(bm, cp, cp);
			bmT.MakeTransparent(clrTransparent);

			// Force cleanup
			gEdge.Dispose();
			gT.Dispose();
			return bmT;
		}
	}
}
