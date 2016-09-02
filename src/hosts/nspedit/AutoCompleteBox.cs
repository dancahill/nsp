using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace NSPEdit
{
	public class AutoCompleteBox : ComboBox
	{
		RichCodeBox richCodeBox1;
		public string nsnamespace;

		public AutoCompleteBox(RichCodeBox richCodeBox1)
		{
			this.richCodeBox1 = richCodeBox1;
			this.DropDownStyle = ComboBoxStyle.DropDown;
			this.AutoCompleteSource = AutoCompleteSource.ListItems;
			//this.AutoCompleteMode = AutoCompleteMode.SuggestAppend;
			this.FlatStyle = FlatStyle.Flat;
			this.Font = new Font("Courier New", 10, FontStyle.Regular);
			this.ForeColor = Color.DarkCyan;
			this.BackColor = Color.FromArgb(0xCC, 0xFF, 0xFF);
			this.Sorted = false;
			this.Visible = false;
			this.LostFocus += CB_LostFocus;
			this.KeyDown += CB_KeyDown;
			this.KeyPress += CB_KeyPress;
			this.TextChanged += CB_TextChanged;
			this.Name = "AutoCompleteBox";
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
				if (this.SelectedIndex < 0) this.SelectedIndex = 0;
				richCodeBox1.SelectedText = this.Text;
				richCodeBox1.SelectedText = e.KeyChar.ToString();
				this.Visible = false;
				Program.MainForm.toolTip1.Hide(Program.MainForm.richCodeBox1);
				richCodeBox1.Focus();
				if (e.KeyChar == '.')
				{
					KeyPressEventArgs x = new KeyPressEventArgs('.');
					Program.MainForm.richCodeBox1_KeyPress(null, x);
				}
			}
		}

		void CB_TextChanged(object sender, EventArgs e)
		{
			if (this.Text != "") this.Items[0] = this.Text;

			//Program.Log("cb text='{0}'", this.Text);
			ToolTip toolTip1 = Program.MainForm.toolTip1;
			//toolTip1.Show(t, this, p.X + e.X, p.Y + e.Y + 32, 5000);
			string ns = nsnamespace + (nsnamespace == "" ? "" : ".") + this.Text;
			XmlHelp.XmlHelpEntry xhelp = XmlHelp.findnode(ns);
			toolTip1.ToolTipTitle = "";
			string t = "";
			if (xhelp != null)
			{
				toolTip1.ToolTipTitle = string.Format("({0}) {1}", xhelp.type, xhelp.name);
				if (xhelp.desc != "") t = string.Format("{0}", xhelp.desc);
				else t = xhelp.fullname;
				if (xhelp.parameters != "" || xhelp.returns != "")
				{
					if (xhelp.desc != "") t += "\r\n";
					if (xhelp.parameters != "") t += string.Format("\r\nParameters: {0}", xhelp.parameters);
					if (xhelp.returns != "") t += string.Format("\r\nReturns: {0}", xhelp.returns);
				}
			}
			Point cursorPt = richCodeBox1.GetPositionFromCharIndex(richCodeBox1.SelectionStart);
			cursorPt.X += (int)richCodeBox1.Font.SizeInPoints;
			cursorPt.Y += richCodeBox1.Location.Y;
			//Program.Log("this.Location.X={0} this.Location.Y={1}", this.Location.X, this.Location.Y);
			//Program.Log("Program.MainForm.Location.X={0} Program.MainForm.Location.Y={1}", Program.MainForm.Location.X, Program.MainForm.Location.Y);
			//Program.Log("Program.MainForm.richCodeBox1.Location.X={0} Program.MainForm.richCodeBox1.Location.Y={1}", Program.MainForm.richCodeBox1.Location.X, Program.MainForm.richCodeBox1.Location.Y);
			//if (t != "") toolTip1.Show(t, Program.MainForm, this.Location.X + this.DropDownWidth + 20, this.Location.Y + 150, 5000);
			if (t != "" && !ns.EndsWith(".")) toolTip1.Show(t, Program.MainForm.richCodeBox1, cursorPt.X + this.Width, cursorPt.Y, 5000);
			else toolTip1.Hide(Program.MainForm.richCodeBox1);
		}

		void CB_KeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Enter)
			{
				if (this.SelectedIndex < 0) this.SelectedIndex = 0;
				richCodeBox1.SelectedText = this.Text;
				this.Visible = false;
				Program.MainForm.toolTip1.Hide(Program.MainForm.richCodeBox1);
				richCodeBox1.Focus();
			}
			else if (e.KeyCode == Keys.Escape)
			{
				this.Text = "";
				this.Visible = false;
				richCodeBox1.Focus();
			}
		}

		void CB_LostFocus(object sender, EventArgs e)
		{
			this.Visible = false;
		}
	}
}
