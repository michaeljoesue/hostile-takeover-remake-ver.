using System;
using System.IO;
using System.Collections;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace SpiffLib
{
	/// <summary>
	/// Summary description for AudioFormats.
	/// </summary>
	public class AudioFormats
	{
		// IMA ADPCM, 8 bit, table driven (the game's sound format)

		static int[] s_anStepIndexMap = new int[] {
			0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,
			0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,
			0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,
			0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,
			2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,88,88,
			4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,88,88,88,88,
			6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,88,88,88,88,88,88,
			8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,88,88,88,88,88,88,88,88,
			0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,
			0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,
			0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,
			0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,
			2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,88,88,
			4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,88,88,88,88,
			6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,88,88,88,88,88,88,
			8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,88,88,88,88,88,88,88,88
		};

		static int[] s_anStepFractions8BitEncode = new int[] {
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,4,4,4,5,5,6,6,7,8,8,9,10,11,12,14,15,16,18,20,22,24,26,29,32,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,4,4,5,5,5,6,7,7,8,9,10,11,12,13,14,15,17,19,20,22,25,27,30,33,36,40,44,48,53,58,64,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,4,4,4,5,5,6,6,7,8,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,49,54,60,66,72,79,87,96,106,116,128,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,4,4,5,5,5,5,6,7,8,8,9,10,11,12,14,14,16,18,20,21,23,26,29,32,35,38,42,46,50,56,61,67,74,81,89,98,108,119,131,144,158,174,191,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,3,3,3,3,4,4,5,5,6,6,7,7,8,9,10,11,12,13,15,16,18,19,21,24,26,28,31,34,38,42,46,50,56,61,67,74,81,89,98,108,119,131,144,158,174,192,211,232,255,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,3,3,3,3,3,4,4,4,4,5,5,6,6,8,8,9,9,10,11,13,14,15,16,19,20,23,24,26,30,33,35,39,43,48,53,58,63,70,76,84,93,101,111,123,135,149,164,180,198,218,240,264,290,319,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,5,5,5,5,6,6,8,8,9,9,11,11,12,14,15,17,18,20,23,24,27,29,32,36,39,42,47,51,57,63,69,75,84,92,101,111,122,134,147,162,179,197,216,237,261,288,317,348,383,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,4,4,4,4,4,5,5,5,5,7,7,9,9,11,11,12,12,14,16,18,19,21,23,26,28,32,33,37,42,46,49,54,60,67,74,81,88,98,107,117,130,142,156,172,189,208,229,252,277,305,336,369,406,446,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2,-2,-2,-2,-2,-2,-3,-3,-3,-4,-4,-4,-5,-5,-6,-6,-7,-8,-8,-9,-10,-11,-12,-14,-15,-16,-18,-20,-22,-24,-26,-29,-32,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2,-2,-2,-2,-2,-2,-3,-3,-3,-3,-4,-4,-5,-5,-5,-6,-7,-7,-8,-9,-10,-11,-12,-13,-14,-15,-17,-19,-20,-22,-25,-27,-30,-33,-36,-40,-44,-48,-53,-58,-64,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2,-2,-2,-2,-2,-2,-3,-3,-3,-3,-4,-4,-4,-5,-5,-6,-6,-7,-8,-8,-9,-10,-11,-12,-13,-14,-16,-17,-19,-21,-23,-25,-28,-31,-34,-37,-41,-45,-49,-54,-60,-66,-72,-79,-87,-96,-106,-116,-128,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2,-2,-2,-2,-2,-2,-2,-2,-2,-3,-3,-4,-4,-5,-5,-5,-5,-6,-7,-8,-8,-9,-10,-11,-12,-14,-14,-16,-18,-20,-21,-23,-26,-29,-32,-35,-38,-42,-46,-50,-56,-61,-67,-74,-81,-89,-98,-108,-119,-131,-144,-158,-174,-191,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2,-2,-2,-2,-2,-3,-3,-3,-3,-4,-4,-5,-5,-6,-6,-7,-7,-8,-9,-10,-11,-12,-13,-15,-16,-18,-19,-21,-24,-26,-28,-31,-34,-38,-42,-46,-50,-56,-61,-67,-74,-81,-89,-98,-108,-119,-131,-144,-158,-174,-192,-211,-232,-255,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-3,-3,-3,-3,-3,-4,-4,-4,-4,-5,-5,-6,-6,-8,-8,-9,-9,-10,-11,-13,-14,-15,-16,-19,-20,-23,-24,-26,-30,-33,-35,-39,-43,-48,-53,-58,-63,-70,-76,-84,-93,-101,-111,-123,-135,-149,-164,-180,-198,-218,-240,-264,-290,-319,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-3,-3,-3,-3,-3,-5,-5,-5,-5,-6,-6,-8,-8,-9,-9,-11,-11,-12,-14,-15,-17,-18,-20,-23,-24,-27,-29,-32,-36,-39,-42,-47,-51,-57,-63,-69,-75,-84,-92,-101,-111,-122,-134,-147,-162,-179,-197,-216,-237,-261,-288,-317,-348,-383,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-4,-4,-4,-4,-4,-5,-5,-5,-5,-7,-7,-9,-9,-11,-11,-12,-12,-14,-16,-18,-19,-21,-23,-26,-28,-32,-33,-37,-42,-46,-49,-54,-60,-67,-74,-81,-88,-98,-107,-117,-130,-142,-156,-172,-189,-208,-229,-252,-277,-305,-336,-369,-406,-446
		};

		public static byte[] EncodeImaAdpcmTableDriven8Bit(byte[] abPcm) {
			ArrayList alsEncoded = new ArrayList();

			byte bPredicted = 128;
			int nStepIndex = 0;

			int[] anStep = new int[2];
			for (int i = 0; i < abPcm.Length; i += 2) {
				// Encoded size is a nibble, so 2 per byte

				for (int k = 0; k < 2; k++) {
					anStep[k] = 0;
					if (i + k == abPcm.Length)
						break;
					int nDelta = abPcm[i + k] - bPredicted;
					int iStepSmallest = -1;
					int nDiffSmallest = int.MaxValue;
					for (int j = 0; j < 16; j++) {
						int nStepFraction = s_anStepFractions8BitEncode[j * 89 + nStepIndex];
						if (bPredicted + nStepFraction < 0 || bPredicted + nStepFraction > 255)
							continue;
						int nDiff = Math.Abs(nStepFraction - nDelta);
						if (nDiff <= nDiffSmallest) {
							nDiffSmallest = nDiff;
							iStepSmallest = j;
						}
					}
					anStep[k] = iStepSmallest;
					bPredicted += (byte)s_anStepFractions8BitEncode[iStepSmallest * 89 + nStepIndex];
					nStepIndex = s_anStepIndexMap[iStepSmallest * 89 + nStepIndex];
				}

				// Add 1 byte to output

				alsEncoded.Add((byte)((anStep[0] << 4) | anStep[1]));
			}

			return (byte[])alsEncoded.ToArray(typeof(byte));
		}

	}
	
	class Pcm {
		int m_cBits;
		int m_hz;
		byte[] m_ab;
		short[] m_ash;

		// See http://www.intersrv.com/~dcross/wavio.html for format info, or
		// http://ccrma-www.stanford.edu/courses/422/projects/WaveFormat/

		[StructLayout(LayoutKind.Sequential)]
			struct FourCC {
			public byte b0;
			public byte b1;
			public byte b2;
			public byte b3;

			public FourCC(string str) {
				b0 = (byte)str[0];
				b1 = (byte)str[1];
				b2 = (byte)str[2];
				b3 = (byte)str[3];
			}
		};

		[StructLayout(LayoutKind.Sequential)]
			struct WavFileHeader { 
			public FourCC fccRIFF;
			public uint cbRiffBytes;
			public FourCC fccWAVE;
			public FourCC fccfmt;
			public uint cbfmtBytes;
			public ushort wFormatTag; 
			public ushort nChannels; 
			public uint nSamplesPerSec; 
			public uint nAvgBytesPerSec; 
			public ushort nBlockAlign; 
			public ushort nBitsPerSample; 
			public FourCC fccdata;
			public uint cbdataBytes;
		};

		unsafe public Pcm (string strWav) {
			if (strWav.EndsWith(".wav") || strWav.EndsWith(".WAV")) {
				LoadWavFile(strWav);
			}
			if (strWav.EndsWith(".snd")) {
				LoadSndFile(strWav);
			}
		}

		unsafe void LoadWavFile(string strWav) {
			BinaryReader brdr = new BinaryReader(new FileStream(strWav, FileMode.Open, FileAccess.Read, FileShare.Read));
			byte[] ab = brdr.ReadBytes((int)brdr.BaseStream.Length);
			brdr.Close();

			WavFileHeader *pwavhdr = (WavFileHeader *)Marshal.UnsafeAddrOfPinnedArrayElement(ab, 0);
			bool fOk = true;
			if (pwavhdr->wFormatTag != 1)
				fOk = false;
			if (pwavhdr->nBitsPerSample != 8 && pwavhdr->nBitsPerSample != 16)
				fOk = false;
			if (!fOk)
				throw new Exception("Only support reading 8 / 16 bit raw pcm!");
			m_cBits = pwavhdr->nBitsPerSample;
			m_hz = (int)pwavhdr->nSamplesPerSec;
			int cbSizeType = Marshal.SizeOf(pwavhdr->GetType());
			switch (m_cBits) {
			case 8:
				m_ab = new byte[pwavhdr->cbdataBytes];
				for (int i = 0; i < m_ab.Length; i++)
					m_ab[i] = ab[i + cbSizeType];
				break;

			case 16:
				m_ash = new short[pwavhdr->cbdataBytes / 2];
				for (int i = 0; i < pwavhdr->cbdataBytes; i += 2)
					m_ash[i / 2] = (short)(ab[i + cbSizeType] + ab[1 + i + cbSizeType] * 256);
				break;
			}
		}

		void LoadSndFile(string strSnd) {
			BinaryReader brdr = new BinaryReader(new FileStream(strSnd, FileMode.Open, FileAccess.Read, FileShare.Read));
			byte[] ab = brdr.ReadBytes((int)brdr.BaseStream.Length);
			brdr.Close();

			m_cBits = 8;
			m_hz = 8000;
			m_ab = new byte[ab.Length - 6];
			for (int i = 0; i < m_ab.Length; i++)
				m_ab[i] = ab[i + 6];
		}

		public Pcm(int hz, byte[] ab) {
			m_cBits = 8;
			m_hz = hz;
			m_ab = ab;
		}

		public Pcm(int hz, short[] ash) {
			m_cBits = 16;
			m_hz = hz;
			m_ash = ash;
		}

		public int BitsPerSample {
			get {
        		return m_cBits;
			}
		}

		public int Hertz {
			get {
				return m_hz;
			}
		}

		public byte[] Data8Bit {
			get {
				return (byte[])m_ab.Clone();
			}
			set {
				m_cBits = 8;
				m_ash = null;
				m_ab = (byte[])value.Clone();
			}
		}

		public short[] Data16Bit {
			get {
				return (short[])m_ash.Clone();
			}
			set {
				m_cBits = 16;
				m_ab = null;
				m_ash = (short[])value.Clone();
			}
		}

		public void ConvertTo8Bit() {
			if (m_cBits == 8)
				return;

			// 16 bit signed (-32768 -> 32767) to
			// 8 bit unsigned (0,127==-128,-1 and 128,255==0,127)

			byte[] ab = new byte[m_ash.Length];
			for (int i = 0; i < ab.Length; i++)
				ab[i] = (byte)((m_ash[i] ^ 0x8000) / 256);
			Data8Bit = ab;
		}

		public void SetMaxSize(int cbMax) {
			if (m_cBits == 8) {
				if (m_ab.Length > cbMax) {
					byte[] abT = new byte[32700];
					Array.Copy(m_ab, 0, abT, 0, abT.Length);
					m_ab = abT;
				}
			} else {
				if (m_ash.Length * 2 > cbMax) {
					short[] ashT = new short[cbMax / 2];
					Array.Copy(m_ash, 0, ashT, 0, ashT.Length);
					m_ash = ashT;
				}
			}
		}

		[DllImportAttribute("winmm.dll")]
		private static extern int PlaySoundW(IntPtr pbSound, IntPtr hModule, uint fdwSound);
		const UInt32 SND_SYNC = 0x0000; // play synchronously (default)
		const UInt32 SND_ASYNC = 0x0001; // play asynchronously
		const UInt32 SND_NODEFAULT = 0x0002;  // silence (!default) if sound not found
		const UInt32 SND_MEMORY = 0x0004; // pszSound points to a memory file
		const UInt32 SND_LOOP = 0x0008; // loop the sound until next sndPlaySound
		const UInt32 SND_NOSTOP = 0x0010;  // don't stop any currently playing sound
		const UInt32 SND_NOWAIT = 0x00002000; // don't wait if the driver is busy
		const UInt32 SND_ALIAS = 0x00010000; // name is a registry alias
		const UInt32 SND_ALIAS_ID = 0x00110000; // alias is a predefined ID
		const UInt32 SND_FILENAME = 0x00020000; // name is file name
		const UInt32 SND_RESOURCE = 0x00040004; // name is resource name or atom
		const UInt32 SND_PURGE = 0x0040;
		const UInt32 SND_APPLICATION = 0x0080; // look for application specific association	

		unsafe public void Play() {
			// Fill in the header

			if (m_ab == null && m_ash == null)
				return;

			int cbData = m_cBits == 8 ? m_ab.Length : m_ash.Length * 2;

			WavFileHeader wavhdr;
			wavhdr.fccRIFF = new FourCC("RIFF");
			wavhdr.cbRiffBytes = (uint)(Marshal.SizeOf(typeof(WavFileHeader)) - 8 + cbData);
			wavhdr.fccWAVE = new FourCC("WAVE");
			wavhdr.fccfmt = new FourCC("fmt ");
			wavhdr.cbfmtBytes = 16;
			wavhdr.wFormatTag = 1;
			wavhdr.nChannels = 1;
			wavhdr.nSamplesPerSec = (uint)m_hz;
			wavhdr.nAvgBytesPerSec = (uint)(m_cBits / 8 * m_hz);
			wavhdr.nBlockAlign = (ushort)(m_cBits / 8);
			wavhdr.nBitsPerSample = (ushort)m_cBits;
			wavhdr.fccdata = new FourCC("data");
			wavhdr.cbdataBytes = (uint)cbData;

			// Serialize the data

			byte[] ab = SerializeStructure(null, &wavhdr, Marshal.SizeOf(wavhdr.GetType()));
			switch (m_cBits) {
			case 8:
				ab = SerializeStructure(ab, (void *)Marshal.UnsafeAddrOfPinnedArrayElement(m_ab, 0), m_ab.Length);
				break;

			case 16:
				ab = SerializeStructure(ab, (void *)Marshal.UnsafeAddrOfPinnedArrayElement(m_ash, 0), m_ash.Length * 2);
				break;
			}

			// Play the sound

			IntPtr ptr = Marshal.UnsafeAddrOfPinnedArrayElement(ab, 0);
			int nSuccess = PlaySoundW(ptr, IntPtr.Zero, SND_ASYNC | SND_MEMORY);
			Debug.Assert(nSuccess != 0);
		}

		unsafe static byte[] SerializeStructure(byte[] ab, void *pv, int cb) {
			byte *pb = (byte *)pv;
			byte[] abNew = null;
			if (ab == null) {
				abNew = new byte[cb];
				for (int i = 0; i < cb; i++)
					abNew[i] = pb[i];
			} else {
				abNew = new byte[ab.Length + cb];
				for (int i = 0; i < ab.Length; i++)
					abNew[i] = ab[i];
				for (int i = 0; i < cb; i++)
					abNew[i + ab.Length] = pb[i];
			}

			return abNew;
		}

		public byte[] GetSndEncoding() {
			ConvertTo8Bit();
			SetMaxSize(32700 * 2);
			return AudioFormats.EncodeImaAdpcmTableDriven8Bit(Data8Bit);
		}

#if false
#endif
	}
}
