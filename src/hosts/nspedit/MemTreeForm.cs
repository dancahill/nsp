using System;
using System.Diagnostics;
using System.Windows.Forms;

namespace NSPEdit
{
	enum NSPObjectTypes
	{
		NT_NULL = 0,
		NT_BOOLEAN = 1,
		NT_NUMBER = 2,
		NT_STRING = 3,
		NT_NFUNC = 4,
		NT_CFUNC = 5,
		NT_TABLE = 6,
		NT_CDATA = 7
	}

	public partial class MemTreeForm : Form
	{
		public ScriptThread thread;

		public MemTreeForm()
		{
			InitializeComponent();
			Load += MemTreeForm_Load;
		}

		private void MemTreeForm_Load(object sender, EventArgs e)
		{

			treeView1.BeginUpdate();
			treeView1.Nodes.Clear();
			TreeNode globalNode = treeView1.Nodes.Add("Globals");
			TreeNode localNode = treeView1.Nodes.Add("Locals");
			TreeNode thisNode = treeView1.Nodes.Add("this");
			if (thread != null)
			{
				LoadSubBranch(globalNode, thread.N.GetGlobal(), 0);
				globalNode.Tag = thread.N.GetGlobal();
				LoadSubBranch(localNode, thread.N.GetLocal(), 0);
				localNode.Tag = thread.N.GetLocal();
				LoadSubBranch(thisNode, thread.N.GetThis(), 0);
				thisNode.Tag = thread.N.GetThis();
			}
			treeView1.EndUpdate();
			treeView1.AfterSelect += TreeView1_AfterSelect;
		}

		private void TreeView1_AfterSelect(object sender, TreeViewEventArgs e)
		{
			TreeNode node = e.Node;
			if (node.Tag != null && node.Tag.GetType() == typeof(NSPObject))
			{
				NSPObject obj = (NSPObject)node.Tag;
				textBox1.Text = string.Format("Type: {0}\r\nValue: {1}", (NSPObjectTypes)obj.type, obj.value);
			}
			else
			{
				textBox1.Text = "";
			}
		}

		void LoadSubBranch(TreeNode parentNode, NSPObject parent, short depth)
		{
			if (depth > 10) return;
			if (parent.type == (short)NSPObjectTypes.NT_TABLE)
			{
				NSPObject listobj = parent.GetFirst();
				while (listobj.IsValid())
				{
					if ((NSPObjectTypes)listobj.type == NSPObjectTypes.NT_TABLE)
					{
						//Trace.WriteLine(string.Format("name: {0}, type: {1}", listobj.name, (NSPObjectTypes)listobj.type));
						TreeNode node = parentNode.Nodes.Add(listobj.name);
						node.Tag = listobj;
						LoadSubBranch(node, listobj, (short)(depth + 1));
					}
					listobj = listobj.GetNext();
				}
				listobj = parent.GetFirst();
				while (listobj.IsValid())
				{
					if ((NSPObjectTypes)listobj.type != NSPObjectTypes.NT_TABLE)
					{
						//Trace.WriteLine(string.Format("name: {0}, type: {1}", listobj.name, (NSPObjectTypes)listobj.type));
						string name = listobj.name;
						if ((NSPObjectTypes)listobj.type == NSPObjectTypes.NT_NFUNC || (NSPObjectTypes)listobj.type == NSPObjectTypes.NT_CFUNC)
						{
							name += "()";
						}
						TreeNode node = parentNode.Nodes.Add(name);
						node.Tag = listobj;
						LoadSubBranch(node, listobj, (short)(depth + 1));
					}
					listobj = listobj.GetNext();
				}
			}
		}
	}
}
