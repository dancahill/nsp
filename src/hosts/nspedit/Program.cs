﻿using System;
using System.Windows.Forms;

namespace NSPEdit
{
	static class Program
	{
		static public NSPEditForm MainForm;
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			MainForm = new NSPEditForm();
			if (args.Length > 0)
			{
				bool getfilename = false;
				foreach (string arg in args)
				{
					if (getfilename)
					{
						getfilename = false;
						MainForm.loadfile = arg;
					}
					if (arg == "-f")
					{
						getfilename = true;
						continue;
					}
					else if (!arg.StartsWith("-") && MainForm.loadfile == "")
					{
						MainForm.loadfile = arg;
					}
				}
			}
			Application.Run(MainForm);
		}

		static public void Log(string format, params object[] list)
		{
			string methname = new System.Diagnostics.StackFrame(1, true).GetMethod().Name;
			string o = string.Format("{0} {1}(): {2}", DateTime.Now, methname, string.Format(format, list));
			if (Program.MainForm != null)
			{
				MainForm.richTextBox2.Text += "\r\n" + o.Substring(o.IndexOf(' ') + 1).Trim();
				MainForm.richTextBox2.SelectionStart = MainForm.richTextBox2.Text.Length;
				MainForm.richTextBox2.ScrollToCaret();
			}
			Application.DoEvents();
		}
	}
}
