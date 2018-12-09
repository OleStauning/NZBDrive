using Microsoft.Win32;
using NZBDrive.Model;
using NZBDrive.WPFMessageView;
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

namespace NZBDrive.View
{
    /// <summary>
    /// Interaction logic for IconToolBar.xaml
    /// </summary>
    public partial class IconToolBar : UserControl
    {
        private NZBDriveModel _model;

        public NZBDriveModel Model
        {
            get { return _model; }
            set
            {
                if (_model != null) throw new ArgumentException("NZBDrive already set");
                _model = value;
                DataContext = _model;
            }
        }

        public IconToolBar()
        {
            InitializeComponent();
        }

        private void LoadNZB(object sender, RoutedEventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Title = "NZBDrive - Open NZB file";
            dialog.Multiselect = true;
            dialog.Filter = "NZB Files (.nzb)|*.nzb|All Files (*.*)|*.*";
            dialog.FilterIndex = 1;
            if (dialog.ShowDialog().Value)
            {
                foreach (var file in dialog.FileNames)
                {
                    _model.Mount(file);
                }
            }
        }

        private void ShowInfoPage(object sender, RoutedEventArgs e)
        {
            var dialog = new AboutWindow();
            dialog.Owner = Window.GetWindow(this);
            dialog.DataContext = _model;

            if (dialog.ShowDialog().Value)
            {
                var res = dialog.MessageView.Result;
            }

/*
            var msgBox = new WPFMessageBox.WPFMessageBox();
            var msgBoxVM = new WPFMessageBox.MessageBoxViewModel(msgBox, "About NZBDrive Usenet Reader", 
                "Find more information on <Hyperlink NavigateUri=\"http://www.nzbdrive.com/license\" "+
                "RequestNavigate=\"Hyperlink_RequestNavigate\">www.nzbdrive.com</Hyperlink>", null,
                WPFMessageBox.WPFMessageBoxButtons.OK, WPFMessageBox.WPFMessageBoxImage.Information);

            msgBox.DataContext = msgBoxVM;
            msgBox.ShowDialog();
*/
/*
            MessageBox.Show(Application.Current.MainWindow,
                "NZBDrive " + _model.CurrentVersion.ToString(3) + ". Copyright © 2015 ByteFountain.\n\n"+
                "Find more information on http://www.nzbdrive.com."
                , "About NZBDrive Usenet Reader", MessageBoxButton.OK, MessageBoxImage.Information);
 */
        }

        private void ShowOptions(object sender, RoutedEventArgs e)
        {
            OptionsWindow dialog = new OptionsWindow();
            dialog.Owner = Window.GetWindow(this);
            dialog.Options = _model.Options;
            if (dialog.ShowDialog().Value)
            {

            }
        }

        public void AddUsenetServer()
        {
            var server = NewsServerConnectionData.NewDefaultConnection;
            var serverEdit = new ServerEditWindow();
            serverEdit.Owner = Window.GetWindow(this);
            serverEdit.DataContext = server;
            bool? res = serverEdit.ShowDialog();
            if (res.Value && serverEdit.MessageView.Result == WPFMessageViewResult.Ok)
            {
                _model.NewsServerCollection.Add(server);
            }        
        }

        private void AddUsenetServer(object sender, RoutedEventArgs e)
        {
            AddUsenetServer();
        }
        
        private void OpenByteFountain(object sender, RoutedEventArgs e)
        {
            System.Diagnostics.Process.Start("http://www.nzbking.com");
        }

        private void GotoDownloadPage(object sender, RoutedEventArgs e)
        {
            System.Diagnostics.Process.Start("http://www.nzbdrive.com/download");
        }

        private void ToolBar_Loaded(object sender, RoutedEventArgs e)
        {
            ToolBar toolBar = sender as ToolBar;
            var overflowGrid = toolBar.Template.FindName("OverflowGrid", toolBar) as FrameworkElement;
            if (overflowGrid != null)
            {
                overflowGrid.Visibility = Visibility.Collapsed;
            }
            var mainPanelBorder = toolBar.Template.FindName("MainPanelBorder", toolBar) as FrameworkElement;
            if (mainPanelBorder != null)
            {
                mainPanelBorder.Margin = new Thickness();
            }
        }

    }

    public class ShowIfFalseValueConverter : IValueConverter
    {
        public object Convert(object val, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return (bool)val ? Visibility.Collapsed : Visibility.Visible;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return null;
        }
    }
    public class ShowIfTrueValueConverter : IValueConverter
    {
        public object Convert(object val, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return (bool)val ? Visibility.Visible : Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return null;
        }
    }

    

}
