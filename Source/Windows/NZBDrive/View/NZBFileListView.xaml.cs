using System;
using System.Collections.Generic;
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
using System.Globalization;
using System.Collections;

namespace NZBDrive.View
{
    [ProvideToolboxControl("General", true)]
    public partial class NZBFileListView : UserControl, IDisposable
    {
        public NZBFileListView()
        {
            InitializeComponent();

            Loaded += NZBFileListView_Loaded;
        }

        void NZBFileListView_Loaded(object sender, RoutedEventArgs e)
        {
            NZBFileList.CollectionChanged += NZBFileList_CollectionChanged;
        }

        private void UnhookEvents()
        {
            Loaded -= NZBFileListView_Loaded;
            NZBFileList.CollectionChanged -= NZBFileList_CollectionChanged;
        }

        void NZBFileList_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            if (e.NewItems != null && e.NewItems.Count > 0)
            {
                FileList.SelectedItem = e.NewItems[0];
            }
        }

        NZBFileList NZBFileList { get { return (NZBFileList)DataContext; } }

        public Action<NZBFile> SelectedNZBFileChanged;

        void ServersList_SelectionChanged(Object sender, SelectionChangedEventArgs e)
        {
            var handler = SelectedNZBFileChanged;
            if (handler == null) return;
            var nzbFile = FileList.SelectedItem as NZBFile;
            handler((NZBFile)FileList.SelectedItem);
        }

        public Action<NZBFile> OpenMountedNZBFile;
        
        void NZBFileListView_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            var handler = OpenMountedNZBFile;
            if (handler == null) return;
            var lvi = e.Source as ListViewItem;
            if (lvi == null) return;
            var nzbFile = lvi.Content as NZBFile;
            if (nzbFile == null) return;
            handler(nzbFile);
        }

        private void MenuItem_Open_Click(object sender, RoutedEventArgs e)
        {
            OpenSelected();
        }

        private void MenuItem_Unmount_Click(object sender, RoutedEventArgs e)
        {
            UnmountSelected();
        }

        private void fileList_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            switch (e.Key)
            {
                case Key.Enter:
                    OpenSelected();
                    break;
                case Key.Delete:
                    UnmountSelected();
                    break;
            }
        }

        private void OpenSelected()
        {
            List<NZBFile> selected = new List<NZBFile>();
            foreach (var file in FileList.SelectedItems) selected.Add((NZBFile)file);

            foreach (var value in selected)
            {
                var handler = OpenMountedNZBFile;
                if (handler == null) return;
                handler(value);
            }
        }

        private void UnmountSelected()
        {
            List<NZBFile> selected = new List<NZBFile>();
            foreach(var file in FileList.SelectedItems) selected.Add((NZBFile)file);

            foreach (var value in selected)
            {
                NZBFileList.Unmount((NZBFile)value);
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

    public class FileStatusConverter : IValueConverter
    {
        private SolidColorBrush OKColor = new SolidColorBrush(Colors.Green);
        private SolidColorBrush WarningColor = new SolidColorBrush(Colors.Yellow);
        private SolidColorBrush ErrorColor = new SolidColorBrush(Colors.Red);

        public object Convert(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            switch ((int)value)
            {
                case (int)NZBFileStatus.OK: return OKColor;
                case (int)NZBFileStatus.Warning: return WarningColor;
                case (int)NZBFileStatus.Error: return ErrorColor;
            }
            return OKColor;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            return null;
        }


    }

    
}
