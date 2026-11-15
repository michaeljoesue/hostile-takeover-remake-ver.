using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Reflection;
using System.Runtime.Serialization;
using System.Text;

// M's document file format.
//
// Levels (.ld) and tile collections (.tc) used to be saved with
// BinaryFormatter, which modern .NET has removed. This format stores the same
// object graph, using the classes' own serialization code: ISerializable
// classes write and read their values through GetObjectData and their
// (SerializationInfo, StreamingContext) constructors exactly as before, and
// the other [Serializable] classes have their fields stored by name. So each
// class reads and writes what it always did; only the container changes.
//
// The file is a magic header and one value. A value is a tag byte and its
// data; objects get ids so shared references stay shared. Strings are UTF-8,
// numbers little endian. Types are stored by full name (no assembly).
//
// Old BinaryFormatter files are still read (see DocManager.LoadLegacy in
// Doc.cs); saving always writes this format.

namespace m
{
	public static class DocFormat
	{
		static readonly byte[] s_abMagic = Encoding.ASCII.GetBytes("HTMDOC\r\n\x1a\x01");
		const int knVersion = 1;

		enum Tag : byte {
			Null = 0, Ref, Bool, Byte, SByte, Char, Int16, UInt16, Int32, UInt32,
			Int64, UInt64, Single, Double, Decimal, String, Enum, Array, ArrayList,
			Hashtable, Serializable, Fields, Bitmap, Point, Size, Color, Rectangle,
			DateTime, PointF, SizeF, RectangleF,
		}

		// Is this file in the new format?

		public static bool IsDocFormat(string strFile) {
			using (FileStream stm = new FileStream(strFile, FileMode.Open,
					FileAccess.Read, FileShare.Read)) {
				return IsDocFormat(stm);
			}
		}

		public static bool IsDocFormat(Stream stm) {
			long pos = stm.Position;
			byte[] ab = new byte[s_abMagic.Length];
			int cb = stm.Read(ab, 0, ab.Length);
			stm.Position = pos;
			if (cb != ab.Length)
				return false;
			for (int i = 0; i < ab.Length; i++) {
				if (ab[i] != s_abMagic[i])
					return false;
			}
			return true;
		}

		//
		// Writing
		//

		public static void Save(Stream stm, object ob) {
			BinaryWriter bwtr = new BinaryWriter(stm, new UTF8Encoding(false));
			bwtr.Write(s_abMagic);
			bwtr.Write(knVersion);
			Writer wtr = new Writer(bwtr);
			wtr.WriteValue(ob);
			bwtr.Flush();
		}

		public static void Save(string strFile, object ob) {
			using (FileStream stm = new FileStream(strFile, FileMode.Create,
					FileAccess.Write, FileShare.None)) {
				Save(stm, ob);
			}
		}

		class RefComparer : IEqualityComparer<object> {
			public new bool Equals(object a, object b) {
				return ReferenceEquals(a, b);
			}
			public int GetHashCode(object ob) {
				return System.Runtime.CompilerServices.RuntimeHelpers.GetHashCode(ob);
			}
		}

		class Writer {
			BinaryWriter m_bwtr;
			Dictionary<object, int> m_mpobid = new Dictionary<object, int>(new RefComparer());
			StreamingContext m_ctx = new StreamingContext(StreamingContextStates.File);

			public Writer(BinaryWriter bwtr) {
				m_bwtr = bwtr;
			}

			void WriteString(string str) {
				m_bwtr.Write(str);
			}

			void WriteType(Type type) {
				WriteString(TypeName(type));
			}

			// Returns true if ob was written before (and a reference was written)

			bool WriteRefOrNewId(object ob, Tag tag) {
				int id;
				if (m_mpobid.TryGetValue(ob, out id)) {
					m_bwtr.Write((byte)Tag.Ref);
					m_bwtr.Write(id);
					return true;
				}
				id = m_mpobid.Count;
				m_mpobid.Add(ob, id);
				m_bwtr.Write((byte)tag);
				return false;
			}

			public void WriteValue(object ob) {
				if (ob == null) {
					m_bwtr.Write((byte)Tag.Null);
					return;
				}
				Type type = ob.GetType();

				// Values

				if (type.IsEnum) {
					m_bwtr.Write((byte)Tag.Enum);
					WriteType(type);
					m_bwtr.Write(Convert.ToInt64(ob));
					return;
				}
				switch (Type.GetTypeCode(type)) {
				case TypeCode.Boolean: m_bwtr.Write((byte)Tag.Bool); m_bwtr.Write((bool)ob); return;
				case TypeCode.Byte: m_bwtr.Write((byte)Tag.Byte); m_bwtr.Write((byte)ob); return;
				case TypeCode.SByte: m_bwtr.Write((byte)Tag.SByte); m_bwtr.Write((sbyte)ob); return;
				case TypeCode.Char: m_bwtr.Write((byte)Tag.Char); m_bwtr.Write((ushort)(char)ob); return;
				case TypeCode.Int16: m_bwtr.Write((byte)Tag.Int16); m_bwtr.Write((short)ob); return;
				case TypeCode.UInt16: m_bwtr.Write((byte)Tag.UInt16); m_bwtr.Write((ushort)ob); return;
				case TypeCode.Int32: m_bwtr.Write((byte)Tag.Int32); m_bwtr.Write((int)ob); return;
				case TypeCode.UInt32: m_bwtr.Write((byte)Tag.UInt32); m_bwtr.Write((uint)ob); return;
				case TypeCode.Int64: m_bwtr.Write((byte)Tag.Int64); m_bwtr.Write((long)ob); return;
				case TypeCode.UInt64: m_bwtr.Write((byte)Tag.UInt64); m_bwtr.Write((ulong)ob); return;
				case TypeCode.Single: m_bwtr.Write((byte)Tag.Single); m_bwtr.Write((float)ob); return;
				case TypeCode.Double: m_bwtr.Write((byte)Tag.Double); m_bwtr.Write((double)ob); return;
				case TypeCode.Decimal: m_bwtr.Write((byte)Tag.Decimal); m_bwtr.Write((decimal)ob); return;
				case TypeCode.String: m_bwtr.Write((byte)Tag.String); WriteString((string)ob); return;
				case TypeCode.DateTime:
					m_bwtr.Write((byte)Tag.DateTime);
					m_bwtr.Write(((DateTime)ob).ToBinary());
					return;
				}
				if (ob is Point) {
					Point pt = (Point)ob;
					m_bwtr.Write((byte)Tag.Point); m_bwtr.Write(pt.X); m_bwtr.Write(pt.Y);
					return;
				}
				if (ob is Size) {
					Size siz = (Size)ob;
					m_bwtr.Write((byte)Tag.Size); m_bwtr.Write(siz.Width); m_bwtr.Write(siz.Height);
					return;
				}
				if (ob is Rectangle) {
					Rectangle rc = (Rectangle)ob;
					m_bwtr.Write((byte)Tag.Rectangle);
					m_bwtr.Write(rc.X); m_bwtr.Write(rc.Y); m_bwtr.Write(rc.Width); m_bwtr.Write(rc.Height);
					return;
				}
				if (ob is PointF) {
					PointF pt = (PointF)ob;
					m_bwtr.Write((byte)Tag.PointF); m_bwtr.Write(pt.X); m_bwtr.Write(pt.Y);
					return;
				}
				if (ob is SizeF) {
					SizeF siz = (SizeF)ob;
					m_bwtr.Write((byte)Tag.SizeF); m_bwtr.Write(siz.Width); m_bwtr.Write(siz.Height);
					return;
				}
				if (ob is RectangleF) {
					RectangleF rc = (RectangleF)ob;
					m_bwtr.Write((byte)Tag.RectangleF);
					m_bwtr.Write(rc.X); m_bwtr.Write(rc.Y); m_bwtr.Write(rc.Width); m_bwtr.Write(rc.Height);
					return;
				}
				if (ob is Color) {
					// Keep the color's identity: known colors by name, others by value
					Color clr = (Color)ob;
					m_bwtr.Write((byte)Tag.Color);
					if (clr.IsEmpty) {
						m_bwtr.Write((byte)0);
					} else if (clr.IsKnownColor) {
						m_bwtr.Write((byte)1);
						m_bwtr.Write((int)clr.ToKnownColor());
					} else if (clr.IsNamedColor) {
						m_bwtr.Write((byte)2);
						WriteString(clr.Name);
					} else {
						m_bwtr.Write((byte)3);
						m_bwtr.Write(clr.ToArgb());
					}
					return;
				}

				// Objects

				if (ob is Bitmap) {
					if (WriteRefOrNewId(ob, Tag.Bitmap))
						return;
					MemoryStream stmT = new MemoryStream();
					((Bitmap)ob).Save(stmT, ImageFormat.Png);
					byte[] ab = stmT.ToArray();
					m_bwtr.Write(ab.Length);
					m_bwtr.Write(ab);
					return;
				}
				if (ob is Array) {
					if (WriteRefOrNewId(ob, Tag.Array))
						return;
					Array arr = (Array)ob;
					WriteType(type.GetElementType());
					m_bwtr.Write(arr.Rank);
					for (int n = 0; n < arr.Rank; n++)
						m_bwtr.Write(arr.GetLength(n));
					foreach (object obT in arr)
						WriteValue(obT);
					return;
				}
				if (type == typeof(ArrayList)) {
					if (WriteRefOrNewId(ob, Tag.ArrayList))
						return;
					ArrayList als = (ArrayList)ob;
					m_bwtr.Write(als.Count);
					foreach (object obT in als)
						WriteValue(obT);
					return;
				}
				if (type == typeof(Hashtable)) {
					if (WriteRefOrNewId(ob, Tag.Hashtable))
						return;
					Hashtable ht = (Hashtable)ob;
					m_bwtr.Write(ht.Count);
					foreach (DictionaryEntry de in ht) {
						WriteValue(de.Key);
						WriteValue(de.Value);
					}
					return;
				}
				if (!type.IsSerializable)
					throw new SerializationException("Type " + type.FullName + " is not serializable");
				if (ob is ISerializable) {
					if (WriteRefOrNewId(ob, Tag.Serializable))
						return;
					SerializationInfo info = new SerializationInfo(type, new FormatterConverter());
#pragma warning disable SYSLIB0050, SYSLIB0051
					((ISerializable)ob).GetObjectData(info, m_ctx);
#pragma warning restore SYSLIB0050, SYSLIB0051
					WriteString(info.FullTypeName);
					m_bwtr.Write(info.MemberCount);
					SerializationInfoEnumerator e = info.GetEnumerator();
					while (e.MoveNext()) {
						WriteString(e.Name);
						WriteValue(e.Value);
					}
					return;
				}

				// Plain [Serializable] class or struct: its fields

				if (WriteRefOrNewId(ob, Tag.Fields))
					return;
				WriteType(type);
				List<FieldInfo> aflds = SerializableFields(type);
				m_bwtr.Write(aflds.Count);
				foreach (FieldInfo fld in aflds) {
					WriteString(FieldKey(type, fld));
					WriteValue(fld.GetValue(ob));
				}
			}
		}

		// Instance fields of type and its bases that BinaryFormatter would
		// store: all but [NonSerialized], and none of a base that isn't
		// [Serializable].

		static List<FieldInfo> SerializableFields(Type type) {
			List<FieldInfo> aflds = new List<FieldInfo>();
			for (Type typeT = type; typeT != null && typeT != typeof(object);
					typeT = typeT.BaseType) {
				if (!typeT.IsSerializable)
					break;
				foreach (FieldInfo fld in typeT.GetFields(BindingFlags.Instance |
						BindingFlags.Public | BindingFlags.NonPublic |
						BindingFlags.DeclaredOnly)) {
					if (fld.IsNotSerialized)
						continue;
					aflds.Add(fld);
				}
			}
			return aflds;
		}

		// Fields of base classes are qualified by their class, as base classes
		// can have fields with the same names

		static string FieldKey(Type type, FieldInfo fld) {
			if (fld.DeclaringType == type)
				return fld.Name;
			return fld.DeclaringType.Name + "+" + fld.Name;
		}

		static string TypeName(Type type) {
			if (type.IsArray) {
				int rank = type.GetArrayRank();
				return TypeName(type.GetElementType()) + "[" + new string(',', rank - 1) + "]";
			}
			return type.FullName;
		}

		//
		// Reading
		//

		public static object Load(Stream stm, StreamingContext ctx) {
			if (!IsDocFormat(stm))
				throw new SerializationException("Not an M document");
			BinaryReader brdr = new BinaryReader(stm, new UTF8Encoding(false));
			brdr.ReadBytes(s_abMagic.Length);
			int nVersion = brdr.ReadInt32();
			if (nVersion > knVersion) {
				throw new SerializationException("This file was saved by a newer M (format " +
						nVersion + ")");
			}
			Reader rdr = new Reader(brdr, ctx);
			object ob = rdr.ReadValue();
			rdr.Complete();
			return ob;
		}

		class Reader {
			BinaryReader m_brdr;
			StreamingContext m_ctx;
			List<object> m_aob = new List<object>();
			List<IDeserializationCallback> m_acb = new List<IDeserializationCallback>();

			public Reader(BinaryReader brdr, StreamingContext ctx) {
				m_brdr = brdr;
				m_ctx = ctx;
			}

			// Reserve an id; objects are recorded when complete, so a reference
			// to an object still being read (a cycle) is an error

			int NewId() {
				m_aob.Add(null);
				return m_aob.Count - 1;
			}

			public object ReadValue() {
				Tag tag = (Tag)m_brdr.ReadByte();
				switch (tag) {
				case Tag.Null: return null;
				case Tag.Ref: {
					int id = m_brdr.ReadInt32();
					if (id < 0 || id >= m_aob.Count || m_aob[id] == null)
						throw new SerializationException("Bad object reference " + id);
					return m_aob[id];
				}
				case Tag.Bool: return m_brdr.ReadBoolean();
				case Tag.Byte: return m_brdr.ReadByte();
				case Tag.SByte: return m_brdr.ReadSByte();
				case Tag.Char: return (char)m_brdr.ReadUInt16();
				case Tag.Int16: return m_brdr.ReadInt16();
				case Tag.UInt16: return m_brdr.ReadUInt16();
				case Tag.Int32: return m_brdr.ReadInt32();
				case Tag.UInt32: return m_brdr.ReadUInt32();
				case Tag.Int64: return m_brdr.ReadInt64();
				case Tag.UInt64: return m_brdr.ReadUInt64();
				case Tag.Single: return m_brdr.ReadSingle();
				case Tag.Double: return m_brdr.ReadDouble();
				case Tag.Decimal: return m_brdr.ReadDecimal();
				case Tag.String: return m_brdr.ReadString();
				case Tag.DateTime: return DateTime.FromBinary(m_brdr.ReadInt64());
				case Tag.Enum: {
					Type type = ResolveType(m_brdr.ReadString());
					return Enum.ToObject(type, m_brdr.ReadInt64());
				}
				case Tag.Point: return new Point(m_brdr.ReadInt32(), m_brdr.ReadInt32());
				case Tag.Size: return new Size(m_brdr.ReadInt32(), m_brdr.ReadInt32());
				case Tag.Rectangle:
					return new Rectangle(m_brdr.ReadInt32(), m_brdr.ReadInt32(),
							m_brdr.ReadInt32(), m_brdr.ReadInt32());
				case Tag.PointF: return new PointF(m_brdr.ReadSingle(), m_brdr.ReadSingle());
				case Tag.SizeF: return new SizeF(m_brdr.ReadSingle(), m_brdr.ReadSingle());
				case Tag.RectangleF:
					return new RectangleF(m_brdr.ReadSingle(), m_brdr.ReadSingle(),
							m_brdr.ReadSingle(), m_brdr.ReadSingle());
				case Tag.Color: {
					byte bKind = m_brdr.ReadByte();
					switch (bKind) {
					case 0: return Color.Empty;
					case 1: return Color.FromKnownColor((KnownColor)m_brdr.ReadInt32());
					case 2: return Color.FromName(m_brdr.ReadString());
					default: return Color.FromArgb(m_brdr.ReadInt32());
					}
				}
				case Tag.Bitmap: {
					int id = NewId();
					int cb = m_brdr.ReadInt32();
					byte[] ab = m_brdr.ReadBytes(cb);

					// Bitmap keeps its stream; give it its own
					Bitmap bm = new Bitmap(new MemoryStream(ab));
					m_aob[id] = bm;
					return bm;
				}
				case Tag.Array: {
					int id = NewId();
					Type typeElement = ResolveType(m_brdr.ReadString());
					int rank = m_brdr.ReadInt32();
					int[] acLengths = new int[rank];
					for (int n = 0; n < rank; n++)
						acLengths[n] = m_brdr.ReadInt32();
					Array arr = Array.CreateInstance(typeElement, acLengths);

					// Elements in row-major order, as foreach enumerated them
					int[] anIndex = new int[rank];
					int c = arr.Length;
					for (int i = 0; i < c; i++) {
						int iT = i;
						for (int n = rank - 1; n >= 0; n--) {
							anIndex[n] = iT % acLengths[n];
							iT /= acLengths[n];
						}
						arr.SetValue(ReadValue(), anIndex);
					}
					m_aob[id] = arr;
					return arr;
				}
				case Tag.ArrayList: {
					int id = NewId();
					int c = m_brdr.ReadInt32();
					ArrayList als = new ArrayList(c);
					for (int i = 0; i < c; i++)
						als.Add(ReadValue());
					m_aob[id] = als;
					return als;
				}
				case Tag.Hashtable: {
					int id = NewId();
					int c = m_brdr.ReadInt32();
					Hashtable ht = new Hashtable(c);
					for (int i = 0; i < c; i++) {
						object obKey = ReadValue();
						ht[obKey] = ReadValue();
					}
					m_aob[id] = ht;
					return ht;
				}
				case Tag.Serializable: {
					int id = NewId();
					Type type = ResolveType(m_brdr.ReadString());
					int c = m_brdr.ReadInt32();
					SerializationInfo info = new SerializationInfo(type, new FormatterConverter());
					for (int i = 0; i < c; i++) {
						string strName = m_brdr.ReadString();
						object obValue = ReadValue();
						info.AddValue(strName, obValue, obValue == null ? typeof(object) : obValue.GetType());
					}
					ConstructorInfo ctor = type.GetConstructor(BindingFlags.Instance |
							BindingFlags.Public | BindingFlags.NonPublic, null,
							new Type[] { typeof(SerializationInfo), typeof(StreamingContext) }, null);
					if (ctor == null) {
						throw new SerializationException("No deserialization constructor on " +
								type.FullName);
					}
					object ob;
					try {
						ob = ctor.Invoke(new object[] { info, m_ctx });
					} catch (TargetInvocationException ex) {
						throw new SerializationException("Couldn't read " + type.FullName +
								": " + ex.InnerException.Message, ex.InnerException);
					}
					m_aob[id] = ob;
					AddCallback(ob);
					return ob;
				}
				case Tag.Fields: {
					int id = NewId();
					Type type = ResolveType(m_brdr.ReadString());
					object ob = NewUninitialized(type);
					Dictionary<string, FieldInfo> mpfld = new Dictionary<string, FieldInfo>();
					foreach (FieldInfo fld in SerializableFields(type))
						mpfld[FieldKey(type, fld)] = fld;
					int c = m_brdr.ReadInt32();
					for (int i = 0; i < c; i++) {
						string strName = m_brdr.ReadString();
						object obValue = ReadValue();
						FieldInfo fld;
						if (mpfld.TryGetValue(strName, out fld))
							fld.SetValue(ob, ConvertField(obValue, fld.FieldType));
					}
					m_aob[id] = ob;
					AddCallback(ob);
					return ob;
				}
				default:
					throw new SerializationException("Unknown value tag " + (int)tag);
				}
			}

			void AddCallback(object ob) {
				IDeserializationCallback cb = ob as IDeserializationCallback;
				if (cb != null)
					m_acb.Add(cb);
			}

			// After the whole graph is read, as BinaryFormatter did

			public void Complete() {
				foreach (IDeserializationCallback cb in m_acb)
					cb.OnDeserialization(null);
			}
		}

		static object ConvertField(object obValue, Type typeField) {
			if (obValue == null || typeField.IsInstanceOfType(obValue))
				return obValue;
			if (typeField.IsEnum)
				return Enum.ToObject(typeField, obValue);
			return Convert.ChangeType(obValue, typeField);
		}

		static object NewUninitialized(Type type) {
#if NETFRAMEWORK
			return FormatterServices.GetUninitializedObject(type);
#else
			return System.Runtime.CompilerServices.RuntimeHelpers.GetUninitializedObject(type);
#endif
		}

		static Dictionary<string, Type> s_mptype = new Dictionary<string, Type>();

		static Type ResolveType(string strName) {
			Type type;
			if (s_mptype.TryGetValue(strName, out type))
				return type;

			if (strName.EndsWith("]")) {
				int ich = strName.LastIndexOf('[');
				Type typeElement = ResolveType(strName.Substring(0, ich));
				int rank = strName.Length - ich - 1;
				type = rank == 1 ? typeElement.MakeArrayType() : typeElement.MakeArrayType(rank);
			} else {
				Assembly[] aasm = {
					typeof(DocFormat).Assembly, typeof(object).Assembly,
					typeof(Bitmap).Assembly, typeof(ArrayList).Assembly,
				};
				foreach (Assembly asm in aasm) {
					type = asm.GetType(strName);
					if (type != null)
						break;
				}
				if (type == null)
					type = Type.GetType(strName);
			}
			if (type == null)
				throw new SerializationException("Unknown type " + strName);
			s_mptype[strName] = type;
			return type;
		}
	}
}
