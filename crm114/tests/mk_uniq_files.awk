#
# generate unique test sets for classification testing...
#

#
# input variables:
#
# debug=1
# verbose=1
# filepath=<directory>
# ...
#


#
# Generated testcases:
#   - generate N bad, N good.
#   - testcases:
#      1: Each with only one unique word per message
#      2: Write 1 common word and one unique word per message
#      3: Write 2 common words and one unique word per message
#      4: Write 5 common words and one unique word per message
#      5: Write 10 common words and one unique word per message
#      6: Write 20 common words and one unique word per message
#      7: Write 50 common words and one unique word per message
#      8: Write 100 common words and one unique word per message
#      9: Write 1 common paragraphs and INJECT one unique word per message
#     10: Write 2 common paragraphs and INJECT one unique word per message
#     11: Write 5 common paragraphs and INJECT one unique word per message
#     12: Write 10 common paragraphs and INJECT one unique word per message
#


# cliff_rand.awk --- generate Cliff random numbers (from the GAWK manual)
function cliff_rand()
{
	_cliff_seed = (100 * log(_cliff_seed)) % 1;
	if (_cliff_seed < 0)
	{
		_cliff_seed = - _cliff_seed;
	}
	return _cliff_seed;
}



function write2file(str)
{
	if (length(dstfile["base"]) > 0)
	{
		printf("%s\n", str) >> dstfile["base"];
	}
	else
	{
		printf("ERR: dst=0 @ %s\n", str);
	}
}

function write2side(str, side)
{
	if (length(dstfile[side]) > 0)
	{
		printf("%d\t%s\n", side, str) >> dstfile[side];
	}
	else
	{
		printf("ERR @ SIDE=%d: dst=0 @ %s\n", side, str);
	}
}

function close_out_files(dummy,			idx)
{
	for (idx in dstfile)
	{
		if (length(dstfile[idx]) > 0)
		{
			close(dstfile[idx]);
		}
		delete dstfile[idx];
	}
}

function construct_filenames(testcase, subdir, side, testsubcounter,			str, created, c2)
{
	if (debug) printf("construct_filenames(testcase = %d, subdir = '%s', side = %d, testsubcounter = %d)\n", testcase, subdir, side, testsubcounter);

	created = 0;

	if (testsubcounter >= 0)
	{
		str = sprintf("%s/test%08d/%s/%04u/%07u.uniq.txt", filepath, testcase, subdir, 1 + filenumber / 1000, filenumber);
		filenumber++;
	
		gsub("//", "/", str);
	
		if (dstfile["base"] != str)
		{
			# close old file:
			if (length(dstfile["base"]) > 0)
			{
				close(dstfile["base"]);
			}

			# assign new file:
			dstfile["base"] = str;
			created = 1;
	
			sub(/\/[^\/]*$/, "", str);
			if (length(str) > 0) 
			{
				# optimization: don't call system(mkdir) more often then necessary: much faster this way!
				if (cached_dirpath["base"] != str)
				{
					mkrecdir(str);
					if (verbose) printf("\n");
					cached_dirpath["base"] = str;
				}
			}

			# make sure all test files start with THREE newlines! (Faking email with no headers...)
			write2file("\n\n");
		}
	}
	c2 = 0;
	if (created || testsubcounter < 0)
	{
		str = sprintf("%s/test%08d/%s.txt", filepath, testcase, subdir);

		gsub("//", "/", str);

		if (dstfile[side] != str)
		{
			# close old file:
			if (length(dstfile[side]) > 0)
			{
				close(dstfile[side]);
			}

			# assign new file:
			dstfile[side] = str;
			c2 = 1;
	
			sub("/[^/]*$", "", str);
			if (length(str) > 0)
			{
				# optimization: don't call system(mkdir) more often then necessary: much faster this way!
				if (cached_dirpath[side] != str)
				{
					mkrecdir(str);
					if (verbose) printf("\n");
					cached_dirpath[side] = str;
				}
			}
		}
	}
		
	# dump each filepath in the 'side' file for future use:
	if (created)
	{
		write2side(dstfile["base"], side);
	}
	
	if (debug) printf("construct_filenames: created = %d, c2 = %d\n", created, c2);

	if (!debug && (created || c2)) printf(".");
}

function mkrecdir(path, mydir)
{
	mydir = path;
	sub("/[^/]*$", "", path);
	if (length(path) > 0 && path != ".." && path != "." && path != "/")
	{
		# if (debug) printf("recursive: mkdir: %s\n", path);
		mkrecdir(path);
	}

	# if (debug) printf("mkdir: %s\n", mydir);
	# if (!debug) printf("#");
	system("mkdir \"" mydir "\" 2> /dev/null");
}


function prep_test_gen(testcase, testsubcounter, state, side, line, 			subdir, i)
{
	subdir = (side == 0 ? "good" : "bad");
	
	if (debug) printf("### prep_test_gen(testcase = %d, testsubcounter = %d, state = %d, side = %d, line = '%s', subdir = '%s')\n", testcase, testsubcounter, state, side, line, subdir);

	if (state == 0)
	{
		for (side = 0; side < 2; side++)
		{
			construct_filenames(testcase, subdir, side, -1);
		
			# start of test pattern for this test side:
			test_message[side] = "";
			test_message_length[side] = 0;
		}	

		state = 1;
	}
	else if (state == 1)
	{
		# prep for side X:
		side = (testsubcounter % 2);

		if (debug) printf("prep_test_gen(state): collect_count[testcase=%d] = %d, test_message_length[side=%d] = %d\n", testcase, collect_count[testcase], side, test_message_length[side]);

		if (testcase >= 1 && testcase <= 8)
		{
			if (test_message_length[side] < collect_count[testcase])
			{
				# collect common words for all subcases:
				test_message[side] = test_message[side] "\n" line;
				test_message_length[side]++;

				if (debug) printf("prep_test_gen(1-8): test_message[side=%d] = '%s', test_message_length[side=%d] = %d\n", side, test_message[side], side, test_message_length[side]);
			}
			else
			{
				if (   (test_message_length[0] >= collect_count[testcase]) \
				    && (test_message_length[1] >= collect_count[testcase]))
				{
					state = 11;
				}
			}
		}
		else if (testcase >= 9 && testcase <= 12)
		{
			if (test_message_length[side] < collect_count[testcase])
			{
				# fetch paragraphs from array:
				i = int(lorem_size * cliff_rand());
				
				test_message[side] = test_message[side] "\n" line ": " lorem[i];
				test_message_length[side]++;

				if (debug) printf("prep_test_gen(9-12): index = %d, test_message[side=%d] = '%s', test_message_length[side=%d] = %d\n", i, side, test_message[side], side, test_message_length[side]);
			}
			else
			{
				if (   (test_message_length[0] >= collect_count[testcase]) \
				    && (test_message_length[1] >= collect_count[testcase]))
				{
					state = 11;
				}
			}
		}
		else if (testcase >= 13 && testcase <= 16)
		{
			if (test_message_length[side] < collect_count[testcase])
			{
				# fetch paragraphs from array:
				i = int(lorem_asian_size * cliff_rand());
				
				# test_message[side] = test_message[side] "\n" line ": " lorem_asian[i];
				test_message[side] = test_message[side] "" line "" lorem_asian[i];
				test_message_length[side]++;

				if (debug) printf("prep_test_gen(13-16): index = %d, test_message[side=%d] = '%s', test_message_length[side=%d] = %d\n", i, side, test_message[side], side, test_message_length[side]);
			}
			else
			{
				if (   (test_message_length[0] >= collect_count[testcase]) \
				    && (test_message_length[1] >= collect_count[testcase]))
				{
					state = 11;
				}
			}
		}
		else if (testcase >= 17 && testcase <= 23)
		{
			# only common words for both sides:
			if (test_message_length[0] < collect_count[testcase])
			{
				# collect common words for all subcases:
				test_message[0] = test_message[0] "\n" line;
				test_message[1] = test_message[1] "\n" line;
				test_message_length[0]++;
				test_message_length[1]++;

				if (debug) printf("prep_test_gen(17-23): test_message = '%s', test_message_length = %d\n", test_message[0], test_message_length[0]);
			}
			else
			{
				if (   (test_message_length[0] >= collect_count[testcase]) \
				    && (test_message_length[1] >= collect_count[testcase]))
				{
					state = 11;
				}
			}
		}
		else if (testcase >= 24 && testcase <= 31)
		{
			# mostly common words for both sides; 1 word per side
			if (test_message_length[side] < collect_count[testcase])
			{
				# inject side-specific word right smack in the middle
				if (test_message_length[side] == int(collect_count[testcase] / 2))
				{
					# collect common words for all subcases:
					test_message[side] = test_message[side] "\n" line;
					test_message_length[side]++;
	
					if (debug) printf("prep_test_gen(24-31): test_message[side=%d] = '%s', test_message_length[side=%d] = %d/%d\n", side, test_message[side], side, test_message_length[side], collect_count[testcase]);
				}
				else
				{
					# collect common words for all subcases:
					test_message[0] = test_message[0] "\n" line;
					test_message[1] = test_message[1] "\n" line;
					test_message_length[0]++;
					test_message_length[1]++;
	
					if (debug) printf("prep_test_gen(24-31): test_message = '%s', test_message_length = %d/%d\n", test_message[0], test_message_length[0], collect_count[testcase]);
				}
			}
			else
			{
				if (   (test_message_length[0] >= collect_count[testcase]) \
				    && (test_message_length[1] >= collect_count[testcase]))
				{
					state = 11;
				}
			}
		}
		else
		{
			state = 11;
		}
	}
	else
	{
		state = 11;
	}

	return state;
}
			
function test_gen(testcase, testsubcounter, state, side, line,				subdir)
{
	subdir = (side == 0 ? "good" : "bad");
	
	if (debug) printf("### test_gen(testcase = %d, testsubcounter = %d, state = %d, side = %d, line = '%s', subdir = '%s')\n", testcase, testsubcounter, state, side, line, subdir);

	if (testsubcounter >= max_test_samples)
	{
		state = 10; # testcase++;
	}
	else
	{
		construct_filenames(testcase, subdir, side, testsubcounter);
	
		# Asian: without word separators!
		if (testcase >= 13 && testcase <= 16)
		{
			write2file(test_message[side] "" line);
		}
		else
		{
			write2file(test_message[side] "\n" line);
		}
	
		if (state != 12)
		{
			state++;
		}
		else
		{
			state = 11;
		}
	}

	return state;
}



BEGIN	{
     	_cliff_seed = 0.1;

	filenumber = 1;
	filename = "";
	state = 0;
	side = 0;
	testsubcounter = 0;

	#
	# (optional) command line arguments:
	#
        if (length(debug) == 0)
	{
		printf("default debug\n");
		debug = 0;
	}
	printf("debug = %d\n", debug);
        if (length(verbose) == 0)
	{
		printf("default verbose\n");
		verbose = 0;
	}
	printf("verbose = %d\n", verbose);
        if (length(max_test_samples) == 0 || ((0 + max_test_samples) < 1))
	{
		printf("default max_test_samples\n");
		max_test_samples = 50;
	}
	printf("max_test_samples = %d\n", max_test_samples);
	# see which and how many testcases we have to generate.
        if (length(testcase) == 0 || ((0 + testcase) < 1))
	{
		printf("default testcase\n");
		testcase = 1;
	}
	printf("testcase = %d\n", testcase);
        if (length(max_testcase) == 0 || ((0 + max_testcase) > 31))
	{
		printf("default max_testcase\n");
		max_testcase = 31;
	}
	printf("max_testcase = %d\n", max_testcase);
	if (length(filepath) == 0)
	{
		printf("default filepath\n");
		filepath = ".";
	}
	printf("filepath = '%s'\n", filepath);

	# convert MSDOS paths to UNIX/generic paths (Win32 supports / as path separator as well, so no harm there)
	gsub("\\", "/", filepath);
	# remove optional trailing slash:
	gsub("/$", "", filepath);

	# dstfile["base"] = "";
	# cached_dirpath["base"] = "";

	# Lorem Ipsum (http://www.lorem-ipsum.info/generator3) data for faked messages:
	i = 0;
	lorem[i++] = "Lorem ipsum te vix doctus sanctus oportere, solum liberavisse delicatissimi est ne. Qui verear nominavi apeirian ad, ut quo dolorum vituperatoribus impedit, quo possim comprehensam ei. Et audiam persius quo, quo congue docendi mediocrem no. Sea ne ferri summo accusamus, duo nonummy suscipit officiis no. Ornatus assentior nec id.";

	lorem[i++] = "Volumus ancillae per in. Mel ad natum adversarium, eu sit rebum aeterno. Solum nostrum scripserit no vel, at latine electram definitionem pri, mel at summo aperiri alterum. Ubique concludaturque in pro, eum essent habemus dissentias id. Vix ne duis sanctus delicatissimi. Cum eu eligendi nominati consequat, nec amet mandamus accommodare eu.";

	lorem[i++] = "Has an ubique offendit scripserit. Duis augue pri in. His ne ipsum perfecto, vim facilisis rationibus ad. Porro corrumpit concludaturque sea ex, pri minim partem ex. Sed mediocrem persequeris ei, quot accommodare eu eum, no nulla audire vis.";

	lorem[i++] = "Nec vero concludaturque at, natum tantas eos ei, sit dicit perpetua ex. Ne ius idque sanctus feugait, paulo saepe salutandi duo te, solet mandamus ex has. Urbanitas temporibus vel ne. Sea an falli possit scaevola.";

	lorem[i++] = "Ad deseruisse definitionem ius. Eu est nibh option detraxit, assentior referrentur cum ne. Per liber populo perpetua cu, no vix novum dicit. Eu eros pertinax mei.";

	lorem[i++] = "Ad sit commune incorrupte. Ei vim eirmod fierent, puto graeco referrentur qui no, ne nam offendit suavitate. Ceteros molestiae cu quo, fugit ullum fabulas his no. Quo no duis fabellas, et ius facilis appareat. At harum malorum omnesque sed.";

	lorem[i++] = "Est corrumpit neglegentur deterruisset in. Mea electram omittantur ad, ex ponderum takimata sit. Vel saepe doming explicari at, est ut convenire rationibus, qui labore expetendis at. Eos an erant eripuit partiendo, eu pro primis delicata elaboraret. Quo rationibus consetetur persequeris ex, amet temporibus ea has.";

	lorem[i++] = "Id pri nostrum mnesarchum, duo errem fabellas ne. Aperiam equidem laboramus mea in. Iracundia disputationi ex eos, ex nam autem novum, quot semper senserit ex vel. Cu vix qualisque deterruisset definitiones. Ea legere mandamus eos, ne nonummy nominavi quo, at gloriatur disputationi his. Et cum nobis equidem disputationi.";

	lorem[i++] = "Eam dictas inermis vituperata te, nam sint homero facete cu, sit an nemore qualisque reprimique. Vel at consul tractatos consectetuer, qui eros eligendi scaevola cu. Quo scaevola electram conceptam at. Eu natum ipsum eam. Eam tale ponderum cu, te minimum laboramus incorrupte sed.";

	lorem[i++] = "Te per congue mediocrem vituperata, nec malorum facilis electram ut. Graecis eleifend appellantur quo id. Ne cum dolore definiebas. Usu ei eius assueverit, fierent postulant ad has. Ius at legimus pericula.";

	lorem_size = i;

	if (debug) printf("lorem_size = %d\n", lorem_size);

	i = 0;
	lorem_asian[i++] = "シトを ベシック レイティングサ ウェ, 功久 セシビリ のイベント ジェントのアクセシ 拡張可, エム ブコンテ シビリティガ ベルの仕と信 健二仕 功久 レンス交 マイクロソフト ルにするために にする, ビリティ ルのアク とセマンティック めよう んア, ップに プリファ アクセシビ サイト作成のヒント のな どら 健二仕 ブコンテ ビスとレイティ リティガイドライン, エム ツアク シビリティ トとして使 オサリングツル, トワク サイト作成のヒント ルのアク シン可な 寛会 ベシック キュメント をリンクテキス ンツア をマ, 併団イ プリファ テストスイト オサリングツル ィに セシビ ネッ シン可な セシビリ の再形式化, ウェ ラベラ ビリティ ンタネット協会";

	lorem_asian[i++] = "イビ 拡張可 ベシック ガイドライン, どら ップに シン可な コンテン ベルの仕と信 ンテ ク付け びつける レンス交, クアップ コンテンツアクセ 内准剛 での の徴 オブジェク クセシビリティ クセス, 拡張可 ウェブア ビリティ ジェントのアクセシ クほ ウェ びつける ウェブオント ツアク ネッ 併団イ クアップ およびそのマ コンテンツアクセ を始めよう ウェブコンテン のため 功久 ベシック トとして使 にする よる";

	lorem_asian[i++] = "ンツア どら シビリティガ クセシビリティ インタラクション, プリファ オサリングツ セシビ パス ハイパ シビリティ ふべからず エム 内准剛 ルビ レンス交 でウェブにと, よる リア式会 レイテリング 拡張可, コンテン 情報セット テキストマ アクセ アク ラベラ での ベシック ガイドライン";

	lorem_asian[i++] = "め「こを マルチメ 情報セット ンツア エム, イビ その他 プリファ ふべからず シビリティガ, んア プリファ レイテリング ディア ロジ ンツアクセシ でウェブにと ルにするために 併団イ, セシビリ レイテリング ベルの仕と信 ンテ ユザエ の徴 アキテクチャ レイティングサ ハイパ, シビリティガ オサリングツル 寛会 ンツア ブコンテ ふべからず どら クセス, ユザエ ンツアクセシ ウェブオント を始めてみよう 寛会 アク 情報セット ふべからず シビリティガ ハイパ, わった での バジョン トモデル ンタネット協会, イビ ップに シビリティ クリック」";

	lorem_asian[i++] = "パス アクセ の再形式化 リティにする, 情報セット オサリングツル クセス ネッ エム ク付け コンテン と会意味 レイティングサ, わった エム プリファ キュメント インフォテ クほ シビリティ プロファイル ユザエ びつける でウェブにと ハイパ ィに その他 クほ の再形式化 クリック」, ンツア 功久 でウェブにと サイトをアクセシブ";

	lorem_asian[i++] = "情報セット ウェブ内容ア プラニングリサチ エム にする, ップに よる シン可な ウェブ内容ア 内准剛 さぁはじ テキストマ イビ, 寛会 その他 スタイル ルにするために ウェブオント ルにするために とセマンティック セシビ んア ルビ 併団イ ティのい コンテン, ビリティ への切りえ セシビリティ 拡張可 をマ, ウェブア を始めよう のため ィに ツアク 丸山亮仕 ウェブ内容ア での, ラベラ まきかずひこ シビリティガ ビスとレイティ 展久";

	lorem_asian[i++] = "オブジェク まきかずひこ ルビ その他, をマ プリファ まきかずひこ クセス エム トワク 丸山亮仕 サイト作成のヒント, ディア ブコンテ ウェブ内容ア エム セシビ らすかる バジョン ングシステム パス, にする エム プリファ びつける, プロセスド ふべからず ディア パス バジョン ふべからず 内准剛 寛会, 併団イ ンテ クリック」 プロファイル";

	lorem_asian[i++] = "スタイル ティのい イドライン 功久 ツアク, の徴 トモデル ベルの仕と信 その他 ツアク パス アクセシビ プロファイル を始めてみよう, の徴 ハイパ マルチメ およびそのマ インタラクション ンテ ップに ウェブ内容ア レイテリング, め「こを リア式会 ボキャブラリ パス めよう での その他 びつける トモデル オブジェク, めよう ティのい ブコンテ プロセスド よる";

	lorem_asian[i++] = "セシビ パス シビリティ トとして使, んア ップに プリファ ンタネット協会 トワク ウェブア キュメント テキストマ の徴, ビリティ のイベント とセマンティック トワク ンテ クほ らすかる イドライン ンタネット協会 その他, の徴 サイト作成のヒント ビスとレイティ テキストマ 拡なマ ップに ウェ め「こを プリファ, 展久 サイト作成のヒント でウェブにと プロトコル のため クアップ プロトコル トワク 功久";

	lorem_asian[i++] = "にする をマ トとして使 リティにする ラベラ ンツアクセシ とセマンティック ウェ, にする ンテ サイト作成のヒント およびそのマ ウェブ内容ア 内准剛 シビリティガ オサリングツ ィに, サイト作成のヒント 丸山亮仕 寛会 その他 のな オブジェク をリンクテキス ラベラ, セシビリ リティにする ベルの仕と信 わった 功久 ンテ 健二仕 サイト作成のヒント テキストマ ングシステム, ルビ およびそのマ セシビリティ サイトをアクセシブ トワク, 展久 めよう インフォテ クリック」";

	lorem_asian_size = i;
	
	# Asian: ditch all word separators, so we can see how we'll do then...
	for (i = 0; i < lorem_asian_size; i++)
	{
		gsub(" ", "", lorem_asian[i]);
	}

	if (debug) printf("lorem_asian_size = %d\n", lorem_asian_size);

	collect_count[1] = 0;
	collect_count[2] = 1;
	collect_count[3] = 2;
	collect_count[4] = 5;
	collect_count[5] = 10;
	collect_count[6] = 20;
	collect_count[7] = 50;
	collect_count[8] = 100;

	collect_count[9]  = 1;
	collect_count[10] = 2;
	collect_count[11] = 5;
	collect_count[12] = 10;

	collect_count[13] = 1;
	collect_count[14] = 2;
	collect_count[15] = 5;
	collect_count[16] = 10;

	collect_count[17] = 1;
	collect_count[18] = 2;
	collect_count[19] = 5;
	collect_count[20] = 10;
	collect_count[21] = 20;
	collect_count[22] = 50;
	collect_count[23] = 100;

	collect_count[24] = 2;
	collect_count[25] = 5;
	collect_count[26] = 10;
	collect_count[27] = 20;
	collect_count[28] = 50;
	collect_count[29] = 100;
	collect_count[30] = 200;
	collect_count[31] = 500;

}



END	{
	close_out_files();
	if (!debug) printf("\n");
}



/.../	{
	if (debug) printf("line: %s\n", $0);
	test_set = 0;
	
	if (testcase > max_testcase)
	{
		exit;
	}

	if (state == 0 || state == 10)
	{
		if (state == 10)
		{
			testcase++;

			if (!debug) printf("%d", testcase);
			
			state = 0;
		}

		if (testcase > max_testcase)
		{
			exit;
		}

		state = prep_test_gen(testcase, testsubcounter, state, side, $0);
		
		if (state == 0)
		{
			side++;
		}
		else
		{
			testsubcounter = 0;
			side = 0;
		}
	}
	else if (state < 10)
	{
		old_state = state;

		state = prep_test_gen(testcase, testsubcounter, state, side, $0);

		testsubcounter++;

		if (state != old_state)
		{
			testsubcounter = 0;
			side = 0;
		}
	}
	else
	{
		old_state = state;

		side = (testsubcounter % 2);

		state = test_gen(testcase, testsubcounter, state, side, $0);

		if (state != old_state)
		{
			testsubcounter++;
		}
	}

	next;
}




