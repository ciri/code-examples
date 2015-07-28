package parser;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.List;


import org.apache.commons.io.FileUtils;

import entities.DataParser;
import entities.ForumPost;

/**
 * Aggregates all mesage files to 1 big CSV file contianing UID, Message and stems:
 * 
 * Stemming:
 * 	 Take all crawl results and chunk them in subfiles DIR_CHUNK/i.txt
 *   Remove special characters, authornames etc to DIR_PARSE/i.txt
 *   Stem the messages to DIR_CHUNK/i.txt.stemmed
 *   Aggregate everthing
 * @author ejdfortu
 */
public class ParseMessages {
	private static String DIR_OUTPUT= "output/";
	private static String DIR_CRAWL = "output/crawled/";
	private static String DIR_CHUNK = "output/chunked/";
	private static String DIR_PARSE = "output/parsed/";
	private static int MAX_CHUNKS = 500;
	
	public static void main(String[] args) throws Exception {
		createAggregateFile2();
		if(true) return;
		
		while(true) {
			cleanUp();
			copyMessages();
			chunkMessages();
			parseChunkedMessages();
			createAggregateFile();
			cleanUp();
			System.out.println("Sleeping 30 minutes");
			Thread.sleep(30*60*1000);
		}
	}
	/********************************************************************************************************
	 *
	 * 		Utility functions
	 * 
	 *********************************************************************************************************/
	public static void copyMessages() throws IOException {
		FileUtils.deleteDirectory(new File(DIR_CRAWL));
		FileUtils.copyDirectory(new File("data"), new File(DIR_CRAWL));
	}
	public static void cleanUp() throws IOException {
		FileUtils.deleteDirectory(new File(DIR_CRAWL));
	} //TODO: implement
	
	/* 
	   Write results to short format [USERID, THREADID, MESSAGE] 
	   In order to ensure mapping back, remember as well[USERID,USERNAME]		
	*/
	public static void createAggregateFile2() throws IOException {	
		List<String> topics = new ArrayList<String>();
		List<String> users = new ArrayList<String>();
		
		FileOutputStream fstream = new FileOutputStream(DIR_OUTPUT+"dataset_users.csv");
	  	BufferedWriter out = new BufferedWriter(new OutputStreamWriter(fstream,"ASCII"));		
	  	
		FileOutputStream fstream2 = new FileOutputStream(DIR_OUTPUT+"dataset_short.csv");
	  	BufferedWriter out2 = new BufferedWriter(new OutputStreamWriter(fstream2,"ASCII"));		
	  	
	  	for(int i = 0; i < MAX_CHUNKS; i++) {
	  		System.out.println("Parsing chunk "+i);
			File f = new File(DIR_PARSE + i + ".txt.stemmed");
			if(f.exists()) {
				FileInputStream lstream = new FileInputStream(f);
				DataInputStream in = new DataInputStream(lstream);
				String strLine="";
				
				while ((strLine = in.readLine()) != null) {
					String[] fields = strSplitNoQuotes(strLine);//strLine.split(";");		
					
					if(fields.length == 6) {
						ForumPost fp = new ForumPost(Integer.parseInt(fields[1]),fields[4],fields[3],fields[5]);	
						fp.threadtitle = fields[2];
						
						int topicid = topics.indexOf(fp.threadtitle);
						if(topicid == -1) {
							topics.add(fp.threadtitle);
							topicid = topics.indexOf(fp.threadtitle);
						}
						if(!users.contains(fields[4])) {
							users.add(fields[4]);
							out.write(users.size() + "\t"+fields[4]+"\n");
						}
						out2.write(topicid+";"+fp.posterid+";"+fp.inhoud+"\n");
					}
					else
						System.out.print("|");
				}
				in.close();				
			}
			out2.write("\n");			
		}
		out2.close();
		out.close();
	}
	//Old version, less efficient because contains all data
	public static void createAggregateFile() throws IOException {
		
		FileOutputStream fstream2 = new FileOutputStream(DIR_OUTPUT+"dataset.csv");
	  	BufferedWriter out = new BufferedWriter(new OutputStreamWriter(fstream2,"ASCII"));
		for(int i = 0; i < MAX_CHUNKS; i++) {
			File f = new File(DIR_PARSE + i + ".txt.stemmed");
			if(f.exists()) {
				FileInputStream fstream = new FileInputStream(f);
				DataInputStream in = new DataInputStream(fstream);
				String strLine="";
				while ((strLine = in.readLine()) != null) {
					out.write(strLine+"\n");
				}
				in.close();				
			}
			out.write("\n");			
		}
		out.close();
	}
	/********************************************************************************************************
	 *
	 * 		CHUNKING
	 * 		Chunk in parts of linesize 1000
	 * 
	 *********************************************************************************************************/
	public static void chunkMessages() throws IOException {
		int n_chunks = 0;
		ArrayList<String> lines = new ArrayList<String>();

		System.out.print("Chunking files.");
		for (int i = 614; i > 0; i--) {
			//Does there already exist a crawl result? chunk!
			File crawFile = new File(DIR_CRAWL + "save" + i + ".csv");
			if(crawFile.exists()) {				
					System.out.print(i+" ");					
					FileInputStream fstream = new FileInputStream(crawFile);
					DataInputStream in = new DataInputStream(fstream);
					BufferedReader br = new BufferedReader(new InputStreamReader(in));
					String strLine;
	
					while ((strLine = br.readLine()) != null) {
						lines.add(strLine);
						if(lines.size() == 1000) {
							File chunkFile =new File(DIR_CHUNK+n_chunks+".txt");
							if(!chunkFile.exists()) {
								//Save
								FileOutputStream fstream2 = new FileOutputStream(chunkFile);
							  	BufferedWriter out = new BufferedWriter(new OutputStreamWriter(fstream2,"ASCII"));
							  	for(String line : lines)
							  		out.write(line+"\n");
							  	System.out.println("chunked! "+chunkFile);
							}
							System.out.println(" .");
						  	n_chunks++;
							lines.clear();
						}
					}
				}
		}
		System.out.println(" all chunking completed.");
	}
	/********************************************************************************************************
	 *
	 * 		PARSING
	 * 		Parse chunked files
	 * 
	 *********************************************************************************************************/
	private static ArrayList<String> forbidden = new ArrayList<String>();
	public static void parseChunkedMessages() throws Exception {
		int N_FILES = MAX_CHUNKS;

		int start_i = 0;
		for( ; start_i <= N_FILES; start_i++)
			if(! new File(DIR_PARSE+start_i+".csv").exists())
				break;
		
		System.out.println("Parsing files "+start_i+" to "+N_FILES);
		
		for(int i = start_i; i <= N_FILES;i++) {
			if(new File(DIR_CHUNK+i+".txt").exists()) {
				System.out.println("--- "+start_i+"+"+i+"/"+N_FILES+" ---");
	
				//Stemming in python (catch things like  don't, can't, ...)
				stemChunkFile(DIR_CHUNK+i+".txt");
	
				//Remove weird characters now
				parseChunkedMessages(i);
	
				//Stem again to make sure we didn't miss anything
				stemChunkFile(DIR_PARSE+i+".txt");
			}
		}
	}
	public static void parseChunkedMessages(int i) throws Exception {		  
		  FileInputStream in = new FileInputStream(new File(DIR_CHUNK+i+".txt.stemmed"));
		  BufferedReader br = new BufferedReader(new InputStreamReader(in));
		  String strLine;
		  
		  FileOutputStream fstream2 = new FileOutputStream(DIR_PARSE+i+".txt");
		  BufferedWriter out = new BufferedWriter(new OutputStreamWriter(fstream2,"ASCII"));
		  
		  while ((strLine = br.readLine()) != null)   {
			  //Step 1: Find authornames to remove
			  int 	idx1 = strLine.indexOf(";")		+1;
			  		
			  		idx1 = strLine.indexOf(";",idx1)+1;	
			  		idx1 = strLine.indexOf("\"",idx1)+1;
			  		idx1 = strLine.indexOf("\"",idx1)+1;
			  		idx1 = strLine.indexOf(";",idx1)+1;
			  		idx1 = strLine.indexOf(";",idx1)+1;
			  int idx2 = strLine.indexOf(";",idx1);
			  if(idx2 > idx1+1) {
				  String author = strLine.substring(idx1,idx2);
				  forbidden.add(filterSpecial(author));

				  //Step 2: Remove special characters and author names
				  String strText = strLine.substring(idx2+1);			  
				  strText = filterSpecial(strText);
				  for(String forbid : forbidden)
					  strText.replaceAll(forbid, "");		  			  
				  String outString = strLine.substring(0,idx2) + ";"+strText;
				  out.write(outString+"\n");
			  }
		  }
		  out.close();
	}

	//Remove all special characters (also use regexp to neuralize)
	public static String filterSpecial(String strText) throws Exception {
		  strText = strText.replaceAll(";", " ");
		  strText = strText.replaceAll("['|’]", "");
		  strText = strText.replaceAll("[?!\"--()\\[\\],.•Â°Ã¸®¤¸š£â€šÃ¢â€™´½—–~{©}‘“”<>@/…:â€™]"," ");
		  strText = strText.replaceAll("[ ][ ]+"," ");
		  strText = strText.toLowerCase();
		  return strText;
	}

	public static String stemChunkFile(String chunkfile) throws Exception {	  
		  //Apply python stemming library for Dutch and pipe results
		  String command = "python src/python/stem2.py "+chunkfile;
		  runCMD(command,false);

		  //Read result
		  InputStream imessagestream = DataParser.class.getResourceAsStream("/tmp/tmpmessage.txt.out");
		  BufferedReader imessagein = new BufferedReader(new InputStreamReader(new DataInputStream(imessagestream)));
		  
		  String stemmed 	= "";
		  String strMessage = "";
		  while((strMessage = imessagein.readLine()) != null) {
			  stemmed = stemmed  + " "+strMessage;
		  }
		  imessagestream.close();
		  return stemmed;
	}

  public static void runCMD(String cmd,boolean verbose) {
      try {
          Runtime rt = Runtime.getRuntime();
          Process pr = rt.exec("cmd.exe /c "+cmd);
          if(verbose) {
	            BufferedReader input = new BufferedReader(new InputStreamReader(pr.getInputStream()));
	
	            String line=null;
	
	            while((line=input.readLine()) != null) {
	                System.out.println(line);
	            }
          }
          int exitVal = pr.waitFor();
          if(verbose)
      	   System.out.println("Exited with error code "+exitVal);
         

      } catch(Exception e) {
          System.out.println(e.toString());
          e.printStackTrace();
      }
  }
  //Split a string in pieces, but don't parse anything in quotes
  public static String[] strSplitNoQuotes(String line) {
	  String[] chunks = line.split(";");
	  if(chunks.length > 6) {
		  System.out.println(chunks);
		  
		  //Merge topics that may not have been badly encoded
		  String id = chunks[1];
	
		  int i;
		  String topic="";
		  for(i=2; !topic.endsWith("\""); i++) {
			  topic = topic + chunks[i];
		  }		
		  String date = chunks[i];
		  String author = chunks[i+1];
		  String text ="";
		  for(i = i+2; i < chunks.length;i++) {
			  text += chunks[i];
		  }		  
		  chunks = new String[]{"",id,topic,date,author,text};
	  }
	  return chunks;
  }
}
