import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.Border;
import java.text.SimpleDateFormat;

import org.jfree.data.*;
import org.jfree.chart.*;
import org.jfree.chart.axis.*;
import org.jfree.chart.plot.*;
import org.jfree.data.time.*;

/* Include the basic HTML code needed by appletviewer in order to
   facilitate testing of the applet with the appletviewer utility. */

/*
<applet code="JPlot" archive="jcommon-0.5.2.jar, jfreechart-0.7.0.jar"
"width=750 height=550><param name='name0' value=...><param name=value0 value=...></applet>
*/

public class JPlot extends JApplet {

	/* Applet initialization method. */
	public void init() {
		int i;
		boolean drawRelative= false;
		if(null!=getParameter("relative"))
			drawRelative = true;
		TimeSeries series;
		TimeSeriesCollection dataset = new TimeSeriesCollection();
		for (i = 0; i < 10; i++) {
			series = getTimeSeries(i, drawRelative);
			if (null == series)
				break;
			dataset.addSeries(series);
		}
		JFreeChart chart = createChart(dataset);

		ChartPanel chartPanel = new ChartPanel(chart);

		/* Add the various panels created above to the applet. */
		Container cp = getContentPane();
		 cp.setLayout(new BoxLayout(cp, BoxLayout.Y_AXIS));
		 cp.add(chartPanel);
	}
	private TimeSeries getTimeSeries(int num, boolean drawRelative) {
		String name = "name" + Integer.toString(num);
		String title = getParameter(name), valuestr, avgstr;
		if (null == title)
			 return null;
		double avg = 1;
		name = "avg" + Integer.toString(num);
		avgstr = getParameter(name);
		if(null!=avgstr)
			avg = Double.parseDouble(avgstr);
		if(avg==0)
			avg = 1;
		if(!drawRelative)
			avg = 1;
		TimeSeries series = new TimeSeries(title);
		 name = "value" + Integer.toString(num);
		 valuestr = getParameter(name);
		if (null == valuestr)
			 return series;
		StringTokenizer st = new StringTokenizer(valuestr), st1;
		int mm, dd, yy;
		double value;
		while (st.hasMoreTokens()) {
			st1 = new StringTokenizer(st.nextToken(), ":");
			if (4 != st1.countTokens())
				break;
			yy = Integer.parseInt(st1.nextToken());
			mm = Integer.parseInt(st1.nextToken());
			dd = Integer.parseInt(st1.nextToken());
			value = Double.parseDouble(st1.nextToken())/avg;
			try {
				series.add(new Day(dd, mm, yy), value);
			} catch (Exception exc) {
			}
		}
		return series;
	}

	private JFreeChart createChart(XYDataset dataset) {
		JFreeChart chart =
		    ChartFactory.createTimeSeriesChart(getParameter("title"),
						       "Time",
						       "Value",
						       dataset,
						       true,
						       true,
						       false);
		XYPlot plot = chart.getXYPlot();
		DateAxis axis = (DateAxis) plot.getDomainAxis();
		axis.setTickUnit(new DateTickUnit(DateTickUnit.DAY, 7,
				 new SimpleDateFormat("MMM-dd")));
		axis.setVerticalTickLabels(true);
		if(null!=getParameter("log")) {
			NumberAxis rangeAxis = new LogarithmicAxis("Log(y)");
			plot.setRangeAxis(rangeAxis);
		}
		 return chart;

	}
	/* Start the applet. */ public void start() {
	} 
/*	public static void main(String[]args) {
		JPlot app = new JPlot();
		 app.init();
		 app.start();
		Frame frame = new Frame();
		 frame.add(app);
		 frame.addWindowListener(new MyWindowListener(app));
		 frame.setSize(200, 150);
		 frame.show();
	}
	public static class MyWindowListener extends WindowAdapter {

		JApplet applet;

		public MyWindowListener(JApplet applet) {
			this.applet = applet;
		}
		public void windowClosing(WindowEvent event) {
			applet.stop();
			applet.destroy();
			System.exit(0);
		}
	}
*/

}
