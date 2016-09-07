using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using System.Xml;

namespace NSPEdit
{
	public class XmlHelp
	{
		public class XmlHelpEntry
		{
			public string fullname = "";
			public string name = "";
			public string type = "";
			public string desc = "";
			public string parameters = "";
			public string returns = "";
		}

		private static string filename1 = Path.GetDirectoryName(Application.ExecutablePath) + @"\NSPNameSpace.xml";
		private static string filename2 = Directory.GetCurrentDirectory() + @"\NSPNameSpace.xml";
		private static XmlDocument xdoc1 = null;
		private static XmlDocument xdoc2 = null;
		private static DateTime lastload = DateTime.Now.AddMinutes(-1);

		private static void loadfiles()
		{
			if (DateTime.Now < lastload.AddMinutes(1)) return;
			lastload = DateTime.Now;
			if (xdoc1 == null)
			{
				if (File.Exists(filename1))
				{
					xdoc1 = new XmlDocument();
					try
					{
						xdoc1.Load(filename1);
					}
					catch
					{
						xdoc1 = null;
						//Program.Log("{0} failed to load", filename1);
					}
				}
			}
			if (xdoc2 == null)
			{
				if (File.Exists(filename2))
				{
					xdoc2 = new XmlDocument();
					try
					{
						xdoc2.Load(filename2);
					}
					catch
					{
						xdoc2 = null;
						//Program.Log("{0} failed to load", filename2);
					}
				}
			}
		}

		public static XmlHelpEntry findnode(string parentnamespace)
		{
			Func<XmlDocument, string, XmlNode> parseresult = (doc, ns) =>
			{
				if (doc == null) return null;
				XmlNode x = null;
				XmlNode NSPNameSpace = doc.DocumentElement.SelectSingleNode("/NSPNameSpace");
				try
				{
					x = (ns == "") ? NSPNameSpace : NSPNameSpace.SelectSingleNode(ns);
				}
				catch { }
				return x;
			};
			loadfiles();
			XmlNode xnode = null;
			if (parentnamespace != "")
			{
				string childnamespace = parentnamespace.Replace(".", "/");
				if (childnamespace.StartsWith("_GLOBALS")) childnamespace = childnamespace.Substring(8, childnamespace.Length - 8);
				if (childnamespace.StartsWith("/")) childnamespace = childnamespace.Substring(1, childnamespace.Length - 1);
				if (childnamespace.EndsWith("/")) childnamespace = childnamespace.Substring(0, childnamespace.Length - 1);
				xnode = parseresult(xdoc1, childnamespace);
				if (xnode == null) xnode = parseresult(xdoc2, childnamespace);
			}
			if (xnode == null) return null;
			XmlHelpEntry he = new XmlHelpEntry();
			he.fullname = parentnamespace;
			he.name = parentnamespace;
			if (xnode != null)
			{
				he.type = xnode.Attributes["type"] != null ? xnode.Attributes["type"].Value : "";
				he.desc = xnode.Attributes["desc"] != null ? xnode.Attributes["desc"].Value : "";
				he.parameters = xnode.Attributes["params"] != null ? xnode.Attributes["params"].Value : "";
				he.returns = xnode.Attributes["returns"] != null ? xnode.Attributes["returns"].Value : "";
			}
			return he;
		}

		public static List<XmlHelpEntry> getlist(string parentnamespace)
		{
			List<XmlHelpEntry> entries = new List<XmlHelpEntry>();










			//Action<string> addtolist = (x) =>
			//{
			//	if (!entries.Contains(x)) entries.Add(x);
			//};
			Func<string, string, bool> parseresult = (filename, ns) =>
			{
				if (!File.Exists(filename)) return false;
				XmlDocument doc = new XmlDocument();


				try
				{
					doc.Load(filename);
				}
				catch
				{
					doc = null;
				}
				if (doc == null) return false;


				XmlNode NSPNameSpace = doc.DocumentElement.SelectSingleNode("/NSPNameSpace");
				XmlNode x = (ns == "") ? NSPNameSpace : NSPNameSpace.SelectSingleNode(ns);
				if (x != null)
				{
					foreach (XmlNode xnode in x.ChildNodes)
					{
						//						string type = xnode.Attributes["type"].Value;
						//						string desc = xnode.Attributes["description"].Value;
						//Program.MainForm.AppendOutput(string.Format("name='{0}', type='{1}', desc='{2}'\r\n", xnode.Name, type, desc));
						//						addtolist(xnode.Name);
						XmlHelpEntry he = new XmlHelpEntry();
						he.fullname = parentnamespace + (parentnamespace == "" ? "" : ".") + xnode.Name;
						he.name = xnode.Name;
						//						if (xnode != null)
						//						{
						//he.type = xnode.Attributes["type"] != null ? xnode.Attributes["type"].Value : "";
						//he.desc = xnode.Attributes["description"] != null ? xnode.Attributes["description"].Value : "";

						he.type = xnode.Attributes["type"] != null ? xnode.Attributes["type"].Value : "";
						he.desc = xnode.Attributes["desc"] != null ? xnode.Attributes["desc"].Value : "";
						he.parameters = xnode.Attributes["params"] != null ? xnode.Attributes["params"].Value : "";
						he.returns = xnode.Attributes["returns"] != null ? xnode.Attributes["returns"].Value : "";
						//		}
						bool found = false;
						foreach (XmlHelpEntry entry in entries) if (entry.name == he.name) found = true;
						if (!found) entries.Add(he);
					}
					if (entries.Count != 0) return true;
				}
				return false;
			};
			//autoCompleteList = new List<string>();
			if (parentnamespace != "")
			{
				string childnamespace = parentnamespace.Replace(".", "/");
				if (childnamespace.StartsWith("_GLOBALS")) childnamespace = childnamespace.Substring(8, childnamespace.Length - 8);
				if (childnamespace.StartsWith("/")) childnamespace = childnamespace.Substring(1, childnamespace.Length - 1);
				if (childnamespace.EndsWith("/")) childnamespace = childnamespace.Substring(0, childnamespace.Length - 1);
				parseresult(Path.GetDirectoryName(Application.ExecutablePath) + @"\NSPNameSpace.xml", childnamespace);
				parseresult(Directory.GetCurrentDirectory() + @"\NSPNameSpace.xml", childnamespace);
			}
			//addtolist("gettype");
			//addtolist("length");
			//addtolist("tostring");
			//autoCompleteList.Sort();

			return entries;
		}
	}
}
