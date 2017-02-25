using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using NZBDrive.Model;
using NZBDrive.WPFMessageView;

namespace NZBDrive.View
{
    [ProvideToolboxControl("General", true)]
    public sealed partial class ServerListView : UserControl, IDisposable
    {

        public NewsServerCollection NewsServerCollection
        {
            get { return (NewsServerCollection)this.GetValue(NewsServerCollectionProperty); }
            set { this.SetValue(NewsServerCollectionProperty, value); }
        }
        public static readonly DependencyProperty NewsServerCollectionProperty = DependencyProperty.Register(
          "NewsServerCollection", typeof(NewsServerCollection), typeof(ServerListView), new PropertyMetadata(null));
        

        public ServerListView()
        {
            InitializeComponent();

            Loaded += ServerListView_Loaded;

        }

        void ServerListView_Loaded(object sender, RoutedEventArgs e)
        {
            NewsServerCollection.CollectionChanged += NewsServerCollection_CollectionChanged;
        }

        

        private void UnhookEvents()
        {
            Loaded -= ServerListView_Loaded;
            NewsServerCollection.CollectionChanged -= NewsServerCollection_CollectionChanged;
        }


        void NewsServerCollection_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            if (e.NewItems!=null && e.NewItems.Count > 0)
            {
                ServersList.SelectedItem = e.NewItems[0];
            }
        }

        private void MenuItem_Create_Click(object sender, RoutedEventArgs e)
        {
            var server = NewsServerConnectionData.NewDefaultConnection;
            var serverEdit = new ServerEditWindow();
            serverEdit.Owner = Window.GetWindow(this);
            serverEdit.DataContext = server;
            bool? res=serverEdit.ShowDialog();
            if (res.Value && serverEdit.MessageView.Result == WPFMessageViewResult.Ok)
            {
                NewsServerCollection.Add(server);
            }
        }
        private void MenuItem_Change_Click(object sender, RoutedEventArgs e)
        {
            EditSelected();
        }
        private void MenuItem_Delete_Click(object sender, RoutedEventArgs e)
        {
            DeleteSelected();
        }

        private void ServerListView_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            var server = (NewsServer)((ListViewItem)e.Source).Content;
            int idx = ServersList.ItemContainerGenerator.IndexFromContainer((ListViewItem)e.Source);
            if (idx < 0 || idx >= ServersList.Items.Count) return;
            EditServer(idx, server);
        }

        private void ServersList_KeyDown(object sender, KeyEventArgs e)
        {
            switch (e.Key)
            {
                case Key.Enter:
                    EditSelected();
                    e.Handled = true;
                    break;
                case Key.Delete:
                    DeleteSelected();
                    e.Handled = true;
                    break;
            }

        }

        private void DeleteSelected()
        {
            int idx = ServersList.SelectedIndex;
            if (idx >= 0 && idx < ServersList.Items.Count)
            {
                NewsServerCollection.Remove(idx);
            }
        }
        private void EditSelected()
        {
            int idx = ServersList.SelectedIndex;
            if (idx >= 0 && idx < ServersList.Items.Count)
            {
                EditServer(idx, NewsServerCollection[idx]);
            }
        }


        private void EditServer(int idx, NewsServer server)
        {
            var serverCopy = new NewsServerConnectionData(server.ServerConnection);
            var serverEdit = new ServerEditWindow();
            serverEdit.Owner = Window.GetWindow(this);
            serverEdit.DataContext = serverCopy;
            bool? res = serverEdit.ShowDialog();
            if (res.Value && serverEdit.MessageView.Result == WPFMessageViewResult.Ok)
            {
                NewsServerCollection.Change(idx, serverCopy);
            }
        }

        private void ListView_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            ContextMenu menu = (ContextMenu)Resources["ServersListMenu"];
            if (ServersList.SelectedIndex == -1)
            {
                ((MenuItem)menu.Items[1]).IsEnabled = false;
                ((MenuItem)menu.Items[2]).IsEnabled = false;
            }
            else
            {
                ((MenuItem)menu.Items[1]).IsEnabled = true;
                ((MenuItem)menu.Items[2]).IsEnabled = true;
            }
        }


        private bool _disposed = false;
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        private void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    UnhookEvents();
                }
                _disposed = true;
            }
        }


    }
}
