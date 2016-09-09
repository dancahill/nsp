using System;
using System.Windows.Forms;

namespace NSPEdit
{
	public partial class SearchForm : Form
	{
		public RichCodeBox rcb;
		int lastindex;
		string lastsearch;

		public SearchForm()
		{
			InitializeComponent();
			this.Activated += SearchForm_Activated;
			this.Deactivate += SearchForm_Deactivate;
		}

		private void SearchForm_Deactivate(object sender, EventArgs e)
		{
			this.Opacity = 0.7;
		}

		private void SearchForm_Activated(object sender, EventArgs e)
		{
			this.Opacity = 1;
		}

		public void buttonSearch_Click(object sender, EventArgs e)
		{
			try
			{
				if (SearchBox.Text == "")
				{
					this.Show();
					return;
				}
				if (SearchBox.Text != lastsearch)
				{
					lastsearch = SearchBox.Text;
				}
				lastindex = rcb.SelectionStart;
				if (string.IsNullOrEmpty(rcb.Text)) return;
				int index = rcb.Find(SearchBox.Text, lastindex + 1, rcb.TextLength, RichTextBoxFinds.None);
				if (index > -1) lastindex = index;
				else MessageBox.Show(string.Format("no matches for '{0}'", SearchBox.Text));
			}
			catch (Exception ex) { MessageBox.Show(ex.Message, "Error"); }
		}

		private void buttonCancel_Click(object sender, EventArgs e)
		{
			this.Hide();
		}

		protected override void OnFormClosing(FormClosingEventArgs e)
		{
			e.Cancel = true;
			//base.OnFormClosing(e);
			this.Hide();
		}
	}
}
