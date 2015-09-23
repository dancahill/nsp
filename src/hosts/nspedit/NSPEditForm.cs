using System;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using System.Collections.Generic;

namespace NSPEdit
{
	public partial class NSPEditForm : Form
	{
		// sample code at https://msdn.microsoft.com/en-us/library/aa970779%28v=vs.110%29.aspx
		// useful youtube video https://www.youtube.com/watch?v=Lh5q5oqRleQ
		// another useful video https://www.youtube.com/watch?v=emmAnZP-uB4

		//public string filetoread = "";
		public string loadfile = "";
		ComboBox CB;
		List<string> autoCompleteList = new List<string>();
		//		string[] flist = new string[] { "gettype", "istr", "length" };

		public RichCodeBox richCodeBox1;
		public RichTextBox richTextBox2;

		//public System.Timers.Timer aTimer;
		public Timer aTimer;

		public NSPEditForm()
		{
			InitializeComponent();
			this.Load += Form_Load;
		}

		void Form_Load(object sender, EventArgs e)
		{
			aTimer = new Timer();
			aTimer.Tick += ATimer_Tick;
			aTimer.Interval = 200;
			//aTimer.AutoReset = false;
			aTimer.Enabled = false;

			autoCompleteList.Add("gettype");
			autoCompleteList.Add("istr");
			autoCompleteList.Add("length");
			autoCompleteList.Add("replace");
			autoCompleteList.Add("split");
			autoCompleteList.Add("str");
			autoCompleteList.Add("substring");
			autoCompleteList.Add("tostring");
			autoCompleteList.Add("tolower");
			autoCompleteList.Add("toupper");
			//autoCompleteList.Add("thisisamuchlongernamethantherest");
			tabControl1.SelectedIndexChanged += TabControl1_SelectedIndexChanged;
			tabControl1.DrawItem += TabControl1_DrawItem;
			tabControl1.MouseClick += TabControl1_MouseClick;
			newTabPage();
			//richCodeBox1.load
			richCodeBox1.LoadScript(loadfile);
			this.ActiveControl = this.richCodeBox1;
		}

		private void TabControl1_DrawItem(object sender, DrawItemEventArgs e)
		{
			TabControl tc = (TabControl)sender;
			string tabtext = this.tabControl1.TabPages[e.Index].Text;
			RectangleF rect = new RectangleF();
			rect.Location = new Point(e.Bounds.Left + 12, e.Bounds.Top + 4);
			rect.Size = new Size(e.Bounds.Width - 20, e.Font.Height);
			//e.Graphics.DrawString(this.tabControl1.TabPages[e.Index].Text, e.Font, Brushes.Black, e.Bounds.Left + 12, e.Bounds.Top + 4);
			if (e.Index == tc.SelectedIndex)
			{
				e.Graphics.DrawString("x", e.Font, Brushes.Red, e.Bounds.Right - 15, e.Bounds.Top + 4);
				e.Graphics.DrawString(tabtext, e.Font, Brushes.Black, rect);
			}
			else
			{
				e.Graphics.DrawString("x", e.Font, Brushes.Black, e.Bounds.Right - 15, e.Bounds.Top + 4);
				e.Graphics.DrawString(tabtext, e.Font, Brushes.DarkGray, rect);
			}
			e.DrawFocusRectangle();
		}

		private void TabControl1_MouseClick(object sender, MouseEventArgs e)
		{
			//Looping through the controls.
			for (int i = 0; i < this.tabControl1.TabPages.Count; i++)
			{
				Rectangle r = tabControl1.GetTabRect(i);
				//Getting the position of the "x" mark.
				Rectangle closeButton = new Rectangle(r.Right - 15, r.Top + 4, 9, 7);
				if (closeButton.Contains(e.Location))
				{
					//if (MessageBox.Show("Would you like to Close this Tab?", "Confirm", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
					//{
					this.tabControl1.TabPages.RemoveAt(i);
					break;
					//}
				}
			}
			if (tabControl1.Controls.Count == 0)
			{
				newTabPage();
				SetScriptFileName("Unnamed");
			}
		}

		private void ATimer_Tick(object sender, EventArgs e)
		{
			System.Diagnostics.Stopwatch stopwatch = new System.Diagnostics.Stopwatch();
			stopwatch.Start();
			aTimer.Stop();
			richCodeBox1.RefreshRTF();
			stopwatch.Stop();
			aTimer.Interval = (int)(stopwatch.ElapsedMilliseconds > 200 ? stopwatch.ElapsedMilliseconds : 200);
		}

		private void TabControl1_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (tabControl1.SelectedTab == null) return;
			if (!tabControl1.SelectedTab.Controls.ContainsKey("splitContainer1")) return;
			SplitContainer splitContainer1 = (SplitContainer)tabControl1.SelectedTab.Controls["SplitContainer1"];
			if (splitContainer1.Panel1.Controls.ContainsKey("RichCodeBox1"))
				richCodeBox1 = (RichCodeBox)splitContainer1.Panel1.Controls["RichCodeBox1"];
			if (splitContainer1.Panel2.Controls.ContainsKey("richTextBox2"))
				richTextBox2 = (RichTextBox)splitContainer1.Panel2.Controls["richTextBox2"];
			this.Text = string.Format("NSP Editor - [{0}]", Path.GetFileName(tabControl1.SelectedTab.Tag.ToString()));
			richCodeBox1.Focus();
		}

		void CB_KeyPress(object sender, KeyPressEventArgs e)
		{
			if (Char.IsControl(e.KeyChar))
			{
			}
			else if (Char.IsLetterOrDigit(e.KeyChar))
			{
			}
			else
			{
				if (CB.SelectedIndex < 0) CB.SelectedIndex = 0;
				richCodeBox1.SelectedText = CB.Text;
				richCodeBox1.SelectedText = e.KeyChar.ToString();
				CB.Visible = false;
				richCodeBox1.Focus();
				if (e.KeyChar == '.')
				{
					KeyPressEventArgs x = new KeyPressEventArgs('.');
					richCodeBox1_KeyPress(null, x);
				}
			}
		}

		void CB_TextChanged(object sender, EventArgs e)
		{
			if (CB.Text != "") CB.Items[0] = CB.Text;
		}

		void CB_KeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Enter)
			{
				if (CB.SelectedIndex < 0) CB.SelectedIndex = 0;
				richCodeBox1.SelectedText = CB.Text;
				CB.Visible = false;
				richCodeBox1.Focus();
			}
			else if (e.KeyCode == Keys.Escape)
			{
				CB.Text = "";
				CB.Visible = false;
				richCodeBox1.Focus();
			}
		}

		void CB_LostFocus(object sender, EventArgs e)
		{
			CB.Visible = false;
		}

		void richCodeBox1_KeyPress(object sender, KeyPressEventArgs e)
		{
			if (e.KeyChar == '.')
			{
				CB.Text = "";
				CB.Items.Clear();
				CB.Items.Add("");
				int maxchar = 0;
				foreach (String s in autoCompleteList)
				{
					CB.Items.Add(s);
					if (maxchar < s.Length) maxchar = s.Length;
				}
				CB.Visible = true;
				Point cursorPt = richCodeBox1.GetPositionFromCharIndex(richCodeBox1.SelectionStart);
				cursorPt.X += (int)richCodeBox1.Font.SizeInPoints;
				cursorPt.Y += richCodeBox1.Location.Y;
				CB.Size = new System.Drawing.Size(100, 1);
				CB.DropDownHeight = richCodeBox1.Font.Height * 4;
				CB.Width = (maxchar + 1) * (int)richCodeBox1.Font.SizeInPoints;
				CB.Location = cursorPt;
				CB.BringToFront();
				// CB.DroppedDown = true;
				CB.Show();
				CB.Focus();
				CB.Text = "";
			}
		}

		void richCodeBox1_DragDrop(object sender, DragEventArgs e)
		{
			string[] FileList = (string[])e.Data.GetData(DataFormats.FileDrop, false);

			foreach (string file in FileList)
			{
				string name = file.ToLower().Trim('"');
				if (name.EndsWith(".ns") || name.EndsWith(".nsp"))
				{
					newTabPage();
					richCodeBox1.LoadScript(file);
				}
			}
		}

		void richCodeBox1_KeyUp(object sender, KeyEventArgs e)
		{
			toolStripStatusLabel2.Text = "Line: " + (richCodeBox1.GetLineFromCharIndex(richCodeBox1.SelectionStart) + 1).ToString();
		}

		void richCodeBox1_MouseClick(object sender, MouseEventArgs e)
		{
			toolStripStatusLabel2.Text = "Line: " + (richCodeBox1.GetLineFromCharIndex(richCodeBox1.SelectionStart) + 1).ToString();
		}

		private void newToolStripButton_Click(object sender, EventArgs e)
		{
			newTabPage();
			SetScriptFileName("Unnamed");
		}

		private void openToolStripButton_Click(object sender, EventArgs e)
		{
			OpenFileDialog OFD = new OpenFileDialog();
			OFD.Filter = "NS Files(*.ns)|*.ns";
			if (OFD.ShowDialog() == DialogResult.OK)
			{
				newTabPage();
				richCodeBox1.LoadScript(OFD.FileName);
			}
		}

		private void saveToolStripButton_Click(object sender, EventArgs e)
		{
			string srcfile = tabControl1.SelectedTab.Tag.ToString();

			if (!srcfile.Contains("\\")) srcfile = "";
			if (srcfile == "")
			{
				SaveFileDialog SVD = new SaveFileDialog();
				SVD.Filter = "NS Files(*.ns)|*.ns";
				SVD.FileName = Path.GetFileName("null.ns");
				if (SVD.ShowDialog() == DialogResult.OK)
				{
					srcfile = SVD.FileName;
				}
			}
			if (srcfile != "")
			{
				StreamWriter sw = new StreamWriter(srcfile);
				sw.Write(richCodeBox1.Text);
				sw.Close();
				toolStripStatusLabel1.Text = "Saved " + srcfile;
				SetScriptFileName(srcfile);
			}
		}

		private void runToolStripButton_Click(object sender, EventArgs e)
		{
			richCodeBox1.RunScript("");
		}

		private void newToolStripMenuItem_Click(object sender, EventArgs e)
		{
			newToolStripButton_Click(sender, e);
		}

		private void openToolStripMenuItem_Click(object sender, EventArgs e)
		{
			openToolStripButton_Click(sender, e);
		}

		private void saveToolStripMenuItem_Click(object sender, EventArgs e)
		{
			saveToolStripButton_Click(sender, e);
		}

		private void exitToolStripMenuItem_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void runToolStripMenuItem_Click(object sender, EventArgs e)
		{
			runToolStripButton_Click(sender, e);
		}

		private void helpToolStripButton_Click(object sender, EventArgs e)
		{
			System.Diagnostics.Process.Start("https://nulllogic.ca/nsp/syntax.html");
		}

		private void saveAsHTMLToolStripMenuItem_Click(object sender, EventArgs e)
		{
			string filetoread = tabControl1.SelectedTab.Tag.ToString();
			SaveFileDialog SVD = new SaveFileDialog();
			SVD.Filter = "HTML Files(*.html)|*.html";
			if (filetoread != "")
			{
				//SVD.InitialDirectory = Path.GetDirectoryName(filetoread);
				SVD.FileName = Path.GetFileName(filetoread) + ".html";
			}
			if (SVD.ShowDialog() == DialogResult.OK)
			{
				string x = CodeFormat.AddColour(richCodeBox1, "HTML");
				string y = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n</head>\r\n<body>\r\n<pre>\r\n" + x + "</body>\r\n</html>\r\n";
				//Clipboard.SetText(x);
				File.WriteAllText(SVD.FileName, y);
				toolStripStatusLabel1.Text = "Saved " + filetoread;
			}
		}

		private void saveAsRTFToolStripMenuItem_Click(object sender, EventArgs e)
		{
			string filetoread = tabControl1.SelectedTab.Tag.ToString();
			SaveFileDialog SVD = new SaveFileDialog();
			SVD.Filter = "RTF Files(*.rtf)|*.rtf";
			if (filetoread != "")
			{
				//SVD.InitialDirectory = Path.GetDirectoryName(filetoread);
				SVD.FileName = Path.GetFileName(filetoread) + ".rtf";
			}
			if (SVD.ShowDialog() == DialogResult.OK)
			{
				string x = CodeFormat.AddColour(richCodeBox1, "RTF");
				File.WriteAllText(SVD.FileName, x);
				toolStripStatusLabel1.Text = "Saved " + filetoread;
			}
		}

		private void newTabToolStripMenuItem_Click(object sender, EventArgs e)
		{
			newTabPage();
			SetScriptFileName("Unnamed");
		}

		private void newTabPage()
		{
			TabPage tabPage1 = new TabPage();
			SplitContainer splitContainer1 = new SplitContainer();
			RichTextBox richTextBox2 = new RichTextBox();
			RichCodeBox richCodeBox1 = new RichCodeBox();
			int t = 64;

			//tabControl1.SuspendLayout();
			//((System.ComponentModel.ISupportInitialize)(splitContainer1)).BeginInit();
			//splitContainer1.Panel1.SuspendLayout();
			//splitContainer1.Panel2.SuspendLayout();
			//splitContainer1.SuspendLayout();
			//tabPage1.SuspendLayout();

			richCodeBox1.KeyUp += richCodeBox1_KeyUp;
			richCodeBox1.MouseClick += richCodeBox1_MouseClick;
			richCodeBox1.DragDrop += richCodeBox1_DragDrop;
			richCodeBox1.KeyPress += richCodeBox1_KeyPress;
			//richCodeBox1.KeyDown += richCodeBox1_KeyDown;

			// richTextBox2
			richTextBox2.AcceptsTab = true;
			richTextBox2.Dock = System.Windows.Forms.DockStyle.Fill;
			richTextBox2.Location = new System.Drawing.Point(0, 0);
			richTextBox2.Name = "richTextBox2";
			richTextBox2.ReadOnly = true;
			//			richTextBox2.Size = new System.Drawing.Size(795, 75);
			//richTextBox2.TabIndex = 1;
			richTextBox2.TabStop = false;
			richTextBox2.Text = "";
			richTextBox2.WordWrap = false;

			richTextBox2.SelectionTabs = new int[] { t * 1, t * 2, t * 3, t * 4, t * 5, t * 6 };
			richTextBox2.BackColor = Color.Black;
			richTextBox2.ForeColor = Color.LightGray;
			richTextBox2.Font = new Font("Courier New", 10, FontStyle.Regular);
			richTextBox2.Text = "";

			CB = new ComboBox();
			CB.DropDownStyle = ComboBoxStyle.DropDown;
			CB.AutoCompleteSource = AutoCompleteSource.ListItems;
			CB.AutoCompleteMode = AutoCompleteMode.SuggestAppend;
			CB.FlatStyle = FlatStyle.Flat;
			CB.Font = new Font("Courier New", 10, FontStyle.Regular);
			CB.ForeColor = Color.DarkCyan;
			CB.BackColor = Color.FromArgb(0xCC, 0xFF, 0xFF);
			CB.Sorted = false;
			CB.Visible = false;
			CB.LostFocus += CB_LostFocus;
			CB.KeyDown += CB_KeyDown;
			CB.KeyPress += CB_KeyPress;
			CB.TextChanged += CB_TextChanged;
			splitContainer1.Panel1.Controls.Add(CB);

			// splitContainer1
			splitContainer1.Dock = DockStyle.Fill;
			splitContainer1.Location = new Point(3, 3);
			splitContainer1.Name = "splitContainer1";
			splitContainer1.Orientation = Orientation.Horizontal;
			splitContainer1.Panel1.Controls.Add(richCodeBox1);
			splitContainer1.Panel2.Controls.Add(richTextBox2);
			splitContainer1.FixedPanel = FixedPanel.Panel2;
			//splitContainer1.Size = new System.Drawing.Size(795, 443);
			//splitContainer1.SplitterDistance = 364;
			splitContainer1.Panel2MinSize = 150;
			//splitContainer1.TabIndex = 2;
			splitContainer1.TabStop = false;

			// tabPage1
			tabPage1.Name = "TabPage " + (tabControl1.TabCount + 1).ToString();
			//tabPage1.Text = "Untitled " + (tabControl1.TabCount + 1).ToString();
			tabPage1.Text = "";
			tabPage1.Tag = "Untitled";
			tabPage1.Controls.Add(splitContainer1);
			tabPage1.Location = new Point(4, 22);
			//			tabPage1.Name = "tabPage1";
			tabPage1.Padding = new Padding(0);
			//			tabPage1.Size = new System.Drawing.Size(801, 449);
			//tabPage1.TabIndex = 0;
			tabPage1.TabStop = false;
			tabPage1.UseVisualStyleBackColor = true;

			tabPage1.ToolTipText = tabPage1.Tag.ToString();

			//tabControl1.TabPages.Add(tabPage1);
			tabControl1.Controls.Add(tabPage1);
			tabControl1.SelectedTab = tabPage1;
			tabControl1.ShowToolTips = true;

			//splitContainer1.Panel1.ResumeLayout(false);
			//splitContainer1.Panel2.ResumeLayout(false);
			//((System.ComponentModel.ISupportInitialize)(splitContainer1)).EndInit();
			//splitContainer1.ResumeLayout(false);
			//tabPage1.ResumeLayout(false);

			TabControl1_SelectedIndexChanged(null, null);
			richCodeBox1.LoadScript("");
			splitContainer1.Panel2MinSize = 50;

			//tabControl1.ResumeLayout();
		}

		private void closeTabToolStripMenuItem_Click(object sender, EventArgs e)
		{
			tabControl1.Controls.Remove(tabControl1.SelectedTab);
			if (tabControl1.Controls.Count == 0)
			{
				newTabPage();
				SetScriptFileName("Unnamed");
			}
		}

		public void ClearOutput()
		{
			richTextBox2.Text = "";
		}

		public void AppendOutput(string OutString)
		{
			richTextBox2.Text += OutString;
			Application.DoEvents();
		}

		public void RestartFormatTimer()
		{
			aTimer.Start();
		}

		public string GetScriptFileName()
		{
			string srcfile = tabControl1.SelectedTab.Tag.ToString();
			if (!srcfile.Contains("\\")) srcfile = "";
			return srcfile;
		}

		public void SetScriptFileName(string FileName)
		{
			tabControl1.SelectedTab.Tag = FileName;
			tabControl1.SelectedTab.ToolTipText = FileName;
			tabControl1.SelectedTab.Name = Path.GetFileName(FileName);
			tabControl1.SelectedTab.Text = Path.GetFileName(FileName);
			this.Text = string.Format("NSP Editor - [{0}]", Path.GetFileName(FileName));
		}

		public void SetStatus(string Msg)
		{
			toolStripStatusLabel1.Text = Msg;
		}
	}
}
