using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace NZBDrive.View
{
    public class AutoScrollingListView : ListView
    {
        private ScrollViewer _scrollViewer;

        protected override void OnItemsSourceChanged(System.Collections.IEnumerable oldValue, System.Collections.IEnumerable newValue)
        {
            base.OnItemsSourceChanged(oldValue, newValue);

            if (oldValue as INotifyCollectionChanged != null)
                (oldValue as INotifyCollectionChanged).CollectionChanged -= ItemsCollectionChanged;

            if (newValue as INotifyCollectionChanged == null) return;

            (newValue as INotifyCollectionChanged).CollectionChanged += ItemsCollectionChanged;
        }

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            // Dig out and store a reference to our internal ScrollViewer  
            _scrollViewer = RecursiveVisualChildFinder<ScrollViewer>(this) as ScrollViewer;
        }

        void ItemsCollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (_scrollViewer == null) return;

            if (!_scrollViewer.VerticalOffset.Equals(_scrollViewer.ScrollableHeight)) return;

            UpdateLayout();
            _scrollViewer.ScrollToBottom();
        }

        private static DependencyObject RecursiveVisualChildFinder<T>(DependencyObject rootObject)
        {
            var child = VisualTreeHelper.GetChild(rootObject, 0);
            if (child == null) return null;

            return child.GetType() == typeof(T) ? child : RecursiveVisualChildFinder<T>(child);
        }
    }  
}
